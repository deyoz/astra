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

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

#define ENDL "\r\n"

using namespace ASTRA;
using namespace BASIC;
using namespace EXCEPTIONS;
using namespace std;

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
    "      (airp IS NULL OR airp=:airp) "
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


string NormalizeDocNo(const string& str)
{
  string result;
  string max_num, curr_num;
  for(string::const_iterator i=str.begin(); i!=str.end(); ++i)
    if (IsDigitIsLetter(*i)) result+=*i;
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

  return (max_num.size()<6)?result:max_num;
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
  create_apis_file(point_id);

  puts("create_apis successfully completed");
  return 0;
};

void create_apis_file(int point_id)
{
  try
  {
  	TQuery Qry(&OraSession);
    Qry.SQLText =
      "SELECT airline,flt_no,suffix,airp,scd_out,NVL(act_out,NVL(est_out,scd_out)) AS act_out, "
      "       point_num, first_point, pr_tranzit, "
      "       country "
      "FROM points,airps,cities "
      "WHERE points.airp=airps.code AND airps.city=cities.code AND "
      "      point_id=:point_id AND points.pr_del=0 AND points.pr_reg<>0 ";
    Qry.CreateVariable("point_id",otInteger,point_id);
    Qry.Execute();
    if (Qry.Eof) return;

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
      "SELECT airp,scd_in,NVL(act_in,NVL(est_in,scd_in)) AS act_in,country "
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
      "      pax_grp.point_dep=:point_dep AND pax_grp.point_arv=:point_arv AND pr_brd IS NOT NULL ";
    PaxQry.CreateVariable("point_dep",otInteger,point_id);
    PaxQry.DeclareVariable("point_arv",otInteger);

    map<string /*country_arv*/, string /*first airp_arv*/> CBPAirps;

    for(TTripRoute::const_iterator r=route.begin(); r!=route.end(); r++)
    {
      //получим информацию по пункту маршрута
      RouteQry.SetVariable("point_id",r->point_id);
      RouteQry.Execute();
      if (RouteQry.Eof) continue;

      TCountriesRow &country_arv = (TCountriesRow&)base_tables.get("countries").get_row("code",RouteQry.FieldAsString("country"));

      map<string, string>::iterator iCBPAirp=CBPAirps.find(country_arv.code);
      if (iCBPAirp==CBPAirps.end())
        iCBPAirp=CBPAirps.insert(make_pair(country_arv.code, RouteQry.FieldAsString("airp"))).first;
      if (iCBPAirp==CBPAirps.end()) throw Exception("iCBPAirp==CBPAirps.end()");
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
        if (Qry.FieldIsNULL("act_out")) throw Exception("act_out empty (airp_dep=%s)",airp_dep.code.c_str());
        TDateTime act_out_local	= UTCToLocal(Qry.FieldAsDateTime("act_out"),tz_region);

        TAirpsRow &airp_arv = (TAirpsRow&)base_tables.get("airps").get_row("code",RouteQry.FieldAsString("airp"));
      	if (airp_arv.code_lat.empty()) throw Exception("airp_arv.code_lat empty (code=%s)",airp_arv.code.c_str());
      	tz_region=AirpTZRegion(airp_arv.code);
        if (RouteQry.FieldIsNULL("scd_in")) throw Exception("scd_in empty (airp_arv=%s)",airp_arv.code.c_str());
        TDateTime act_in_local = UTCToLocal(RouteQry.FieldAsDateTime("scd_in"),tz_region);

        TAirpsRow &airp_cbp = (TAirpsRow&)base_tables.get("airps").get_row("code",iCBPAirp->second);
      	if (airp_cbp.code_lat.empty()) throw Exception("airp_cbp.code_lat empty (code=%s)",airp_cbp.code.c_str());

        for(;!ApisSetsQry.Eof;ApisSetsQry.Next())
        {
          string fmt=ApisSetsQry.FieldAsString("format");

          string airline_country;
          if (fmt=="TXT_EE")
          {
            if (airline.city.empty()) throw Exception("airline.city empty (code=%s)",airline.code.c_str());
            TCitiesRow &airlineCityRow = (TCitiesRow&)base_tables.get("cities").get_row("code",airline.city);
            TCountriesRow &airlineCountryRow = (TCountriesRow&)base_tables.get("countries").get_row("code",airlineCityRow.country);
   	    	  if (airlineCountryRow.code_iso.empty()) throw Exception("airlineCountryRow.code_iso empty (code=%s)",airlineCityRow.country.c_str());
   	    	  airline_country = airlineCountryRow.code_iso;
          };

          Paxlst::PaxlstInfo FPM(Paxlst::PaxlstInfo::FlightPassengerManifest);
        	Paxlst::PaxlstInfo FCM(Paxlst::PaxlstInfo::FlightCrewManifest);

          if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US")
          {
            for(int pass=0; pass<2; pass++)
            {
              Paxlst::PaxlstInfo& paxlstInfo=(pass==0?FPM:FCM);

              if (fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US")
                paxlstInfo.settings().setRespAgnCode("ZZZ");

              paxlstInfo.settings().setViewUNGandUNE(!(fmt=="EDI_IN" || fmt=="EDI_US"));

              //информация о преставительства a/к
              list<TAirlineOfficeInfo> offices;
              GetAirlineOfficeInfo(airline.code, country_arv.code, airp_arv.code, offices);
              if (!offices.empty())
              {
                paxlstInfo.setPartyName(offices.begin()->contact_name);
                paxlstInfo.setPhone(offices.begin()->phone);
                paxlstInfo.setFax(offices.begin()->fax);
              };

              vector<string> strs;
              vector<string>::const_iterator i;

              SeparateString(string(ApisSetsQry.FieldAsString("edi_own_addr")), ':', strs);
              i=strs.begin();
              if (i!=strs.end()) paxlstInfo.setSenderName(*i++);
              if (i!=strs.end()) paxlstInfo.setSenderCarrierCode(*i++);

              if (fmt=="EDI_US" &&
                  (country_arv.code_lat=="US" ||
                   country_arv.code_lat=="GU"))
              {
                paxlstInfo.setRecipientName("USCSAPIS");
                paxlstInfo.setRecipientCarrierCode("ZZ");
              }
              else if
                 (fmt=="EDI_IN" &&
                  (country_arv.code_lat=="IN"))
              {
                paxlstInfo.setRecipientName("NZCS");
                paxlstInfo.setRecipientCarrierCode("");
              }
              else
              {
                SeparateString(string(ApisSetsQry.FieldAsString("edi_addr")), ':', strs);
                i=strs.begin();
                if (i!=strs.end()) paxlstInfo.setRecipientName(*i++);
                if (i!=strs.end()) paxlstInfo.setRecipientCarrierCode(*i++);
              };

              ostringstream flight;
              flight << airline.code_lat << flt_no << suffix;

              string iataCode = Paxlst::createIataCode(flight.str(),act_in_local);
              paxlstInfo.setIataCode( iataCode );
              if (fmt=="EDI_IN" || fmt=="EDI_US")
                paxlstInfo.setCarrier(airline.code_lat);
              paxlstInfo.setFlight(flight.str());
              paxlstInfo.setDepPort(airp_dep.code_lat);
              paxlstInfo.setDepDateTime(act_out_local);
              paxlstInfo.setArrPort(airp_arv.code_lat);
              paxlstInfo.setArrDateTime(act_in_local);
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
            if (status==psCrew && !(fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US")) continue;
            if (status!=psCrew && !boarded && !(fmt=="EDI_US")) continue;

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

            CheckIn::TPaxDocItem doc;
            CheckIn::TPaxDocoItem doco;
            CheckIn::TPaxDocaItem docaD;
            CheckIn::TPaxDocaItem docaR;
            bool doc_exists=LoadPaxDoc(pax_id, doc);
            bool doco_exists=LoadPaxDoco(pax_id, doco);
            bool docaD_exists=false;
            bool docaR_exists=false;
            if (fmt=="EDI_US")
            {
              docaD_exists=LoadPaxDoca(pax_id, CheckIn::docaDestination, docaD);
              docaR_exists=LoadPaxDoca(pax_id, CheckIn::docaResidence, docaR);
            };

      	    if (!doc_exists)
      	  	{
              doc.surname=transliter(PaxQry.FieldAsString("surname"),1,1);
              doc.first_name=transliter(PaxQry.FieldAsString("name"),1,1);
      	  	  //документ пассажира не найден
              if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US")
              {
        	      paxInfo.setName(doc.first_name);
        	      paxInfo.setSurname(doc.surname);
        	    };
              if (fmt=="CSV_CZ")
        	    {
      	        body << doc.surname << ";"
      	  		       << doc.first_name << ";"
      	  		       << ";;;;;;;;";
      	  		};
      	  		if (fmt=="CSV_DE")
        	    {
      	        body << doc.surname << ";"
      	  		       << TruncNameTitles(doc.first_name) << ";"
      	  		       << ";;;"
                     << airp_arv.code_lat << ";"
      	  		       << airp_dep.code_lat << ";"
      	  		       << airp_final_lat << ";;;";
      	  		};
      	  		if (fmt=="TXT_EE")
      	  		{
      	  		  body << "1# " << count+1 << ENDL
      	  		       << "2# " << doc.surname << ENDL
      	  		       << "3# " << TruncNameTitles(doc.first_name) << ENDL
      	  		       << "4# " << ENDL
      	  		       << "5# " << ENDL
      	  		       << "6# " << ENDL
      	  		       << "7# " << ENDL
      	  		       << "8# " << ENDL
      	  		       << "9# " << ENDL
                     << "10# " << ENDL
                     << "11# " << ENDL;
              };
      	    }
      	    else
      	    {
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
              if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US" || fmt=="CSV_DE" || fmt=="TXT_EE")
              {
                if (!doc_second_name.empty())
                {
                  doc_first_name+=" "+doc_second_name;
                  doc_second_name.clear();
                };
              };

      	      string gender;
      	      if (!doc.gender.empty())
      	      {
      	    	  TGenderTypesRow &gender_row = (TGenderTypesRow&)base_tables.get("gender_types").get_row("code",doc.gender);
      	    	  if (gender_row.code_lat.empty()) throw Exception("gender.code_lat empty (code=%s)",doc.gender.c_str());
      	    	  gender=gender_row.code_lat;
      	    	  if (fmt=="EDI_CZ" || fmt=="EDI_US")
                {
                  gender = gender.substr(0,1);
                  if (gender!="M" &&
                      gender!="F")
        	          gender.clear();
                };
      	    	  if (fmt=="CSV_DE" || fmt=="EDI_CN" || fmt=="EDI_IN")
      	    	  {
      	    	    gender = gender.substr(0,1);
                  if (gender!="M" &&
                      gender!="F")
        	          gender = "U";
        	      };
        	      if (fmt=="TXT_EE")
      	    	  {
      	    	    gender = gender.substr(0,1);
                  if (gender!="M" &&
                      gender!="F")
        	          gender = "N";
        	      };
      	    	};
      	    	string doc_type;
      	    	if (!doc.type.empty())
      	    	{
      	    	  TPaxDocTypesRow &doc_type_row = (TPaxDocTypesRow&)base_tables.get("pax_doc_types").get_row("code",doc.type);
      	    	  if (doc_type_row.code_lat.empty()) throw Exception("doc_type.code_lat empty (code=%s)",doc.type.c_str());
      	    	  doc_type=doc_type_row.code_lat;

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
                         (doc_type=="L")))) doc_type.clear();
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
                         (doc_type=="AC")))) doc_type.clear();
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
                         (doc_type=="AC")))) doc_type.clear();
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
                         (doc_type=="L")))) doc_type.clear();
                };
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
              if (fmt=="EDI_IN" || fmt=="EDI_US")
                doc_no=NormalizeDocNo(doc.no);

              if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US")
              {
                paxInfo.setName(doc_first_name);
        	      paxInfo.setSurname(doc_surname);
                paxInfo.setSex(gender);

        	      if (doc.birth_date!=NoExists)
        	        paxInfo.setBirthDate(doc.birth_date);

                if (fmt=="EDI_US")
                  paxInfo.setCBPPort(airp_cbp.code_lat);
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


                if (fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US")
                {
                  if (!doc_type.empty() && !doc_no.empty())
                  {
                    paxInfo.setDocType(doc_type);
                    paxInfo.setDocNumber(doc_no);
                  }
                }
                else
                {
                  paxInfo.setDocType(doc_type);
                  paxInfo.setDocNumber(doc_no);
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
                if (docaD.region.size()!=2)
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

            if (fmt=="CSV_CZ" || fmt=="CSV_DE")
              body << ENDL;

            if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US")
            {
              if (status!=psCrew)
      	        FPM.addPassenger( paxInfo );
              else
                FCM.addPassenger( paxInfo );
            };
      	  }; //цикл по пассажирам

          vector< pair<string, string> > files;

          if (fmt=="EDI_CZ" || fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US")
          {
            for(int pass=0; pass<2; pass++)
            {
              Paxlst::PaxlstInfo& paxlstInfo=(pass==0?FPM:FCM);

              if (!paxlstInfo.passengersList().empty())
              {
                vector<string> parts;
                string file_extension;
                if (fmt=="EDI_CZ")
                {
                  file_extension="TXT";
                  parts.push_back(paxlstInfo.toEdiString());
                };
                if (fmt=="EDI_CN" || fmt=="EDI_IN" || fmt=="EDI_US")
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
      	    ostringstream file_name;
      	    if (fmt=="CSV_CZ" || fmt=="CSV_DE")
            {
              ostringstream f;
              f << airline.code_lat << flt_no << suffix;
            	file_name << ApisSetsQry.FieldAsString("dir")
                        << airline.code_lat
                        << (f.str().size()<6?string(6-f.str().size(),'0'):"") << flt_no << suffix  //по стандарту поле не должно превышать 6 символов
            	          << airp_dep.code_lat
            	          << airp_arv.code_lat
            	          << DateTimeToStr(scd_out_local,"yyyymmdd") << ".CSV";
            };

            if (fmt=="TXT_EE")
              file_name << ApisSetsQry.FieldAsString("dir")
                        << "LL-" << airline.code_lat << setw(3) << setfill('0') << flt_no << suffix
                        << "-" << DateTimeToStr(act_in_local,"ddmmyyyy-hhnn") << "-S.TXT";

            //доклеиваем заголовочную часть
            ostringstream header;
            if (fmt=="CSV_CZ")
            	header << "csv;ROSSIYA;"
            	    	 << airline.code_lat << setw(3) << setfill('0') << flt_no << suffix << ";"
          	      	 << airp_dep.code_lat << ";" << DateTimeToStr(act_out_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
          	      	 << airp_arv.code_lat << ";" << DateTimeToStr(act_in_local,"yyyy-mm-dd'T'hh:nn:00.0") << ";"
          	      	 << count << ";" << ENDL;
          	if (fmt=="CSV_DE")
              header << airline.code_lat << ";"
                  	 << airline.code_lat << setw(3) << setfill('0') << flt_no << ";"
                  	 << airp_dep.code_lat << ";" << DateTimeToStr(act_out_local,"yymmddhhnn") << ";"
                  	 << airp_arv.code_lat << ";" << DateTimeToStr(act_in_local,"yymmddhhnn") << ";"
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
                  	 << "9$ " << DateTimeToStr(act_in_local,"dd.mm.yy hh:nn") << ENDL
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
        };
      };

    };
  }
  catch(Exception &E)
  {
    throw Exception("create_apis_file: %s",E.what());
  };

};

