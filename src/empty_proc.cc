//---------------------------------------------------------------------------
#include "obrnosir.h"
#include "date_time.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "checkin.h"
#include "passenger.h"
#include "telegram.h"
#include "tlg/tlg_parser.h"
#include "tlg/tlg.h"
#include "tclmon/tcl_utils.h"
#include "serverlib/ourtime.h"
#include <set>
#include "season.h"
#include "sopp.h"
#include "file_queue.h"
#include "events.h"
#include "transfer.h"
#include "term_version.h"
#include "typeb_utils.h"
#include "misc.h"
#include "oralib.h"
#include "httpClient.h"
#include "web_main.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"
#include <boost/utility/in_place_factory.hpp>

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace std;


int season_to_schedules(int argc,char **argv)
{
  //!!!TDateTime first_date = NowUTC()-3000;
  //TDateTime first_date = NowUTC()-500;
  //  TDateTime last_date = NowUTC() + 750;
  try {
//    ConvertSeason( first_date, last_date );
  }
  catch(const EXCEPTIONS::Exception& e){
    ProgError( STDLOG,"EXCEPTIONS::Exception, what=%s", e.what() );
  }
  catch(...){
    ProgError( STDLOG,"unknown error" );
  }
  return 0;
}
/*
CREATE TABLE drop_test_typeb_utils1
(
  point_id NUMBER(9) NOT NULL,
  tlg_id NUMBER(9) NOT NULL,
  type VARCHAR2(6) NOT NULL,
  addr VARCHAR2(1000) NULL,
  addr_normal VARCHAR2(1000) NULL,
  pr_lat NUMBER(1) NOT NULL,
  extra VARCHAR2(50) NULL
);

CREATE TABLE drop_test_typeb_utils2
(
  point_id NUMBER(9) NOT NULL,
  type VARCHAR2(6) NOT NULL,
  addr VARCHAR2(1000) NULL,
  addr_normal VARCHAR2(1000) NULL,
  pr_lat NUMBER(1) NOT NULL,
  extra VARCHAR2(250) NULL
);

CREATE UNIQUE INDEX drop_test_typeb_utils1__IDX ON drop_test_typeb_utils1(tlg_id);

select count(*) from
(
SELECT point_id, type, addr_normal, pr_lat FROM drop_test_typeb_utils1 WHERE type IN
('PTM','PTMN','BTM', 'TPM', 'PSM' , 'PFS', 'PFSN' , 'FTL' , 'PRL', 'PIM', 'SOM', 'ETLD', 'LDM', 'CPM', 'MVTA')
MINUS
SELECT point_id, type, addr_normal, pr_lat FROM drop_test_typeb_utils2
ORDER BY type
);

select count(*) from
(
SELECT point_id, type, addr_normal, pr_lat FROM drop_test_typeb_utils2
MINUS
SELECT point_id, type, addr_normal, pr_lat FROM drop_test_typeb_utils1 WHERE type IN
('PTM','PTMN','BTM', 'TPM', 'PSM' , 'PFS', 'PFSN' , 'FTL' , 'PRL', 'PIM', 'SOM', 'ETLD', 'LDM', 'CPM', 'MVTA')
ORDER BY type
);

*/
//int test_typeb_utils2(int argc,char **argv)
//{
//  TQuery Qry(&OraSession);
//  Qry.Clear();
//  Qry.SQLText="DELETE FROM drop_test_typeb_utils1";
//  Qry.Execute();

//  Qry.Clear();
//  Qry.SQLText="DELETE FROM drop_test_typeb_utils2";
//  Qry.Execute();

//  ASTRA::commit();

//  Qry.Clear();
//  Qry.SQLText=
//    "SELECT airline,flt_no,suffix,airp,scd_out,act_out, "
//    "       point_id,point_num,first_point,pr_tranzit "
//    "FROM points "
////    "WHERE point_id=3977047";
//    "WHERE scd_out BETWEEN TRUNC(SYSDATE-1) AND TRUNC(SYSDATE) AND act_out IS NOT NULL AND pr_del=0";

//  TQuery TlgQry(&OraSession);
//  TlgQry.Clear();
//  TlgQry.SQLText=
//    "INSERT INTO drop_test_typeb_utils1(point_id, tlg_id, type, addr, pr_lat, extra) "
//    "SELECT tlg_out.point_id, tlg_out.id AS tlg_id, tlg_out.type, tlg_out.addr, tlg_out.pr_lat, tlg_out.extra "
//    "FROM tlg_out "
//    "WHERE tlg_out.point_id=:point_id AND tlg_out.num=1 ";
//    //"   AND tlg_out.time_create BETWEEN :act_out-1/1440 AND :act_out+1/1440";
//  TlgQry.DeclareVariable("point_id", otInteger);
//  //TlgQry.DeclareVariable("act_out", otDate);

//  TQuery Tlg2Qry(&OraSession);
//  Tlg2Qry.Clear();
//  Tlg2Qry.SQLText=
//    "INSERT INTO drop_test_typeb_utils2(point_id, type, addr, addr_normal, pr_lat, extra) "
//    "VALUES(:point_id, :type, :addr, :addr, :pr_lat, :extra) ";
//  Tlg2Qry.DeclareVariable("point_id", otInteger);
//  Tlg2Qry.DeclareVariable("type", otString);
//  Tlg2Qry.DeclareVariable("addr", otString);
//  Tlg2Qry.DeclareVariable("pr_lat", otInteger);
//  Tlg2Qry.DeclareVariable("extra", otString);


//  Qry.Execute();
//  for(;!Qry.Eof;Qry.Next())
//  {
//    try
//    {
//      TAdvTripInfo fltInfo(Qry);
//      TlgQry.SetVariable("point_id", fltInfo.point_id);
//      //TlgQry.SetVariable("act_out", Qry.FieldAsDateTime("act_out"));
//      TlgQry.Execute();

//      Tlg2Qry.SetVariable("point_id", fltInfo.point_id);
//      TypeB::TTakeoffCreator creator(fltInfo.point_id);
//      creator << "MVTA";
//      vector<TypeB::TCreateInfo> createInfo;
//      creator.getInfo(createInfo);
//      for(vector<TypeB::TCreateInfo>::const_iterator i=createInfo.begin(); i!=createInfo.end(); ++i)
//      {
//  /*      ProgTrace(TRACE5, "point_id=%d tlg_type=%s addr=%s",
//                          i->point_id, i->get_tlg_type().c_str(), i->get_addrs().c_str());*/

//          //if (!TypeB::TSendInfo(i->get_tlg_type(), fltInfo).isSend()) continue;

//          Tlg2Qry.SetVariable("type", i->get_tlg_type());
//          Tlg2Qry.SetVariable("addr", i->get_addrs());
//          Tlg2Qry.SetVariable("pr_lat", (int)i->get_options().is_lat);
//          localizedstream extra(AstraLocale::LANG_RU);
//          Tlg2Qry.SetVariable("extra", i->get_options().extraStr(extra).str());
//          Tlg2Qry.Execute();
//      };
//    }
//    catch(...)
//    {
//      ASTRA::rollback();
//      ProgTrace(TRACE5, "ERROR! point_id=%d", Qry.FieldAsInteger("point_id"));
//    };
//    ASTRA::commit();
//  };

//  Qry.Clear();
//  Qry.SQLText="SELECT tlg_id, addr FROM drop_test_typeb_utils1";
//  Qry.Execute();

//  TlgQry.Clear();
//  TlgQry.SQLText="UPDATE drop_test_typeb_utils1 SET addr_normal=:addr_normal WHERE tlg_id=:tlg_id";
//  TlgQry.DeclareVariable("addr_normal", otString);
//  TlgQry.DeclareVariable("tlg_id", otInteger);
//  TypeB::TCreateInfo ci;
//  for(;!Qry.Eof;Qry.Next())
//  {
//    ci.set_addrs(Qry.FieldAsString("addr"));
//    TlgQry.SetVariable("tlg_id", Qry.FieldAsInteger("tlg_id"));
//    TlgQry.SetVariable("addr_normal", ci.get_addrs());
//    TlgQry.Execute();
//  };

//  ASTRA::commit();

//  return 0;
//};

void filter(vector<TypeB::TCreateInfo> &createInfo, set<string> tlg_types)
{
  if ( tlg_types.empty() ) {
    return;
  }
    while(true) {
        vector<TypeB::TCreateInfo>::iterator iv = createInfo.begin();
        for(; iv != createInfo.end(); iv++)
            if( tlg_types.find(iv->get_tlg_type()) == tlg_types.end())
                break;
        if(iv != createInfo.end())
            createInfo.erase(iv);
        else
            break;
    }
};

int test_typeb_utils(int argc,char **argv)
{
    string interval = "SYSTEM.UTCSYSDATE-24/24 AND SYSTEM.UTCSYSDATE";
    if(argc == 2) interval = argv[1];
    if(argc > 2) {
        cout << "Usage: " << argv[0] << " \"" << interval << "\"" << endl;
        return 1;
    }

    set<string> tlg_types;
    tlg_types.insert("PRL");
    //  tlg_types.insert("LCI");
    //  tlg_types.insert("COM");
    //  tlg_types.insert("PRL");
    //  tlg_types.insert("PRLC");
    //  tlg_types.insert("PSM");
    //  tlg_types.insert("PIL");
    //  tlg_types.insert("SOM");
    TQuery Qry(&OraSession);
    ofstream f1, f2;
    try
    {
        Qry.Clear();
        string SQLText = (string)
            "SELECT airline,flt_no,suffix,airp,scd_out,act_out, "
            "       point_id,point_num,first_point,pr_tranzit "
            "FROM points "
            //      "WHERE point_id=2253498";
            "WHERE scd_out BETWEEN " + interval + " AND act_out IS NOT NULL AND pr_del=0";

        Qry.SQLText= SQLText;

        DB::TQuery TlgQry(PgOra::getROSession({ "TLG_OUT", "TYPEB_OUT_EXTRA"}), STDLOG);
        string sql =
                "SELECT * "
                "FROM tlg_out "
                "  LEFT OUTER JOIN typeb_out_extra "
                "    ON ("
                "      tlg_out.id = typeb_out_extra.tlg_id "
                "      AND typeb_out_extra.lang = 'EN' "
                "    ) "
                "WHERE "
                " point_id = :POINT_ID "
                " AND manual_creation = 0";

        if ( !tlg_types.empty() ) {
            sql += " AND type IN " + GetSQLEnum(tlg_types);
        }
        sql += " ORDER BY type, typeb_out_extra.text, addr, id, num";
        TlgQry.SQLText=sql;
        TlgQry.DeclareVariable("point_id", otInteger);

        DB::TQuery OrigQry(PgOra::getROSession("TYPEB_ORIGINATORS"), STDLOG);
        OrigQry.SQLText="SELECT * FROM typeb_originators WHERE id=:id";
        OrigQry.DeclareVariable("id", otInteger);

        string file_name;
        file_name="telegram1.txt";
        f1.open(file_name.c_str());
        if (!f1.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
        file_name="telegram2.txt";
        f2.open(file_name.c_str());
        if (!f2.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());

        Qry.Execute();
        for(;!Qry.Eof;Qry.Next())
        {
            TDateTime time_create=NowUTC();
            TAdvTripInfo fltInfo(Qry);
            set<int> tlg_ids;

            for(int pass=0; pass<=1; pass++)
            {
                TlgQry.SetVariable("point_id", fltInfo.point_id);
                TlgQry.Execute();
                if (!TlgQry.Eof)
                {
                    string body;
                    for(;!TlgQry.Eof;)
                    {
                        TTlgOutPartInfo tlg;
                        tlg.fromDB(TlgQry);
                        //originator
                        TypeB::TOriginatorInfo origInfo;
                        OrigQry.SetVariable("id", tlg.originator_id);
                        OrigQry.Execute();
                        if (!OrigQry.Eof)
                            origInfo.fromDB(OrigQry);
                        tlg.origin=origInfo.originSection(time_create, TypeB::endl);

                        body+=tlg.body;
                        TlgQry.Next();
                        if (TlgQry.Eof || tlg.id!=TlgQry.FieldAsInteger("id"))
                        {
                            if (tlg_ids.find(tlg.id)==tlg_ids.end())
                            {
                                if (tlg.tlg_type!="BSM")
                                {
                                    ofstream &f=(pass==0?f1:f2);
                                    f << ConvertCodepage(tlg.addr, "CP866", "CP1251")
                                        << ConvertCodepage(tlg.origin, "CP866", "CP1251")
                                        << ConvertCodepage(tlg.heading, "CP866", "CP1251")
                                        << ConvertCodepage(body, "CP866", "CP1251")
                                        << ConvertCodepage(tlg.ending, "CP866", "CP1251")
                                        << "====================================================" << TypeB::endl;
                                };
                                tlg_ids.insert(tlg.id);
                            };
                            body.clear();
                        };
                    };

                };


                if (pass==0)
                {
                    vector<TypeB::TCreateInfo> createInfo;
                    TypeB::TTakeoffCreator(fltInfo.point_id).getInfo(createInfo);
                    filter(createInfo, tlg_types);
                    TelegramInterface::SendTlg(createInfo);

                    TypeB::TMVTACreator(fltInfo.point_id).getInfo(createInfo);
                    filter(createInfo, tlg_types);
                    TelegramInterface::SendTlg(createInfo);

                    TypeB::TCloseCheckInCreator(fltInfo.point_id).getInfo(createInfo);
                    filter(createInfo, tlg_types);
                    TelegramInterface::SendTlg(createInfo);

                    TypeB::TCloseBoardingCreator(fltInfo.point_id).getInfo(createInfo);
                    filter(createInfo, tlg_types);
                    TelegramInterface::SendTlg(createInfo);
                };
            };


            ASTRA::rollback();
        };
        ASTRA::rollback();
        if (f1.is_open()) f1.close();
        if (f2.is_open()) f2.close();
    }
    catch(EXCEPTIONS::Exception &e)
    {
        try
        {
            ASTRA::rollback();
            if (f1.is_open()) f1.close();
            if (f2.is_open()) f2.close();
            ProgError(STDLOG, "test_typeb_utils2: %s", e.what() );
        }
        catch(...) {};
        throw;
    }
    catch(...)
    {
        try
        {
            ASTRA::rollback();
            if (f1.is_open()) f1.close();
            if (f2.is_open()) f2.close();
        }
        catch(...) {};
        throw;
    };

    return 0;
};

int test_sopp_sql(int argc,char **argv)
{
  boost::posix_time::ptime mcsTime;
  long int delta_exec_old=0, delta_exec_new=0, delta_sql_old=0, delta_sql_new=0;
  long int exec_old=0, exec_new=0, exec_old_sql=0, exec_new_sql=0;
  InitLogTime(argc>0?argv[0]:NULL);
  ofstream f1, f2;
  string file_name;
  file_name="sopp_test_old.txt";
  f1.open(file_name.c_str());
  if (!f1.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
  file_name="sopp_test_new.txt";
  f2.open(file_name.c_str());
  if (!f2.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
  int file_count=0, file_size=0;
  //запрос для нового и старого обработчика, далее сравнения результатов
  int step=0, stepcount=0;
  TReqInfo *reqInfo = TReqInfo::Instance();
  for ( TDateTime day=NowUTC()-3.0; day<=NowUTC()+3.0; day++ ) {
  for ( int iuser_type=0; iuser_type<3; iuser_type++ ) {
    for ( int itime_type=0; itime_type<3; itime_type++ ) {
      for ( int icond=0; icond<=11; icond++ ) {
        reqInfo->Initialize("МОВ");
        switch( itime_type ) {
          case 0:
            reqInfo->user.sets.time = ustTimeLocalDesk;
            break;
          case 1:
            reqInfo->user.sets.time = ustTimeLocalAirp;
            break;
          case 2:
            reqInfo->user.sets.time = ustTimeUTC;
            break;
        }
        switch( iuser_type ) {
          case 0:
            reqInfo->user.user_type = utSupport;
            break;
          case 1:
            reqInfo->user.user_type = utAirline;
            break;
          case 2:
            reqInfo->user.user_type = utAirport;
            break;
        }
        TAccessElems<string> airps, airlines;
        switch( icond ) {
          case 0:
            airps.set_elems_permit(true);
            airlines.set_elems_permit(true);
            break;
          case 1:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(true);
            break;
          case 2:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(false);
            break;
          case 3:
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(true);
            break;
          case 4:
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(false);
            break;
          case 5:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(true);
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(true);
            break;
          case 6:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(false);
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(true);
            break;
          case 7:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(true);
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(false);
            break;
          case 8:
            airps.add_elem( "ДМД" );
            airps.set_elems_permit(false);
            airlines.add_elem( "ЮТ" );
            airlines.set_elems_permit(false);
            break;
          case 9:
            airps.add_elem( "ВНК" );
            airps.add_elem( "СОЧ" );
            airps.add_elem( "BBU" );
            airps.set_elems_permit(true);
            break;
          case 10:
            airlines.add_elem( "ЮТ" );
            airlines.add_elem( "ПО" );
            airlines.add_elem( "НН" );
            airlines.set_elems_permit(true);
            break;
          case 11:
            airps.add_elem( "ВНК" );
            airps.add_elem( "СОЧ" );
            airps.add_elem( "BBU" );
            airps.set_elems_permit(true);
            airlines.add_elem( "ЮТ" );
            airlines.add_elem( "ПО" );
            airlines.add_elem( "НН" );
            airlines.set_elems_permit(true);
            break;
        }
        reqInfo->user.access.merge_airlines(airlines);
        reqInfo->user.access.merge_airps(airps);

          ProgTrace( TRACE5, "user_type=%d, time_type=%d, icond=%d, day=%s, step=%d",
                     iuser_type, itime_type, icond, DateTimeToStr( day, ServerFormatDateTimeAsString ).c_str(), step );
          string strNew, strOld;
          xmlDocPtr ReqDocNew = NULL,
                    ReqDocOld = NULL,
                    ResDocNew = NULL,
                    ResDocOld = NULL;
          stepcount++;
          try {
            ReqDocNew = CreateXMLDoc( "requestNew" );
            ReqDocOld = CreateXMLDoc( "requestOld" );
            xmlNodePtr reqNodeNew = NewTextChild( ReqDocNew->children, "query" );
            xmlNodePtr reqNodeOld = NewTextChild( ReqDocOld->children, "query" );
            NewTextChild( reqNodeNew, "flight_date", DateTimeToStr( day, ServerFormatDateTimeAsString ) );
            NewTextChild( reqNodeNew, "pr_verify_new_select", 1 );
            NewTextChild( reqNodeOld, "flight_date", DateTimeToStr( day, ServerFormatDateTimeAsString ) );
            ResDocNew = CreateXMLDoc( "result" );
            ResDocOld = CreateXMLDoc( "result" );
            xmlNodePtr resNodeNew = NewTextChild( ResDocNew->children, "term" );
            resNodeNew = NewTextChild( resNodeNew, "answer" );
            xmlNodePtr resNodeOld = NewTextChild( ResDocOld->children, "term" );
            resNodeOld = NewTextChild( resNodeOld, "answer" );

            if ( step == 0 ) {
              step = 1;
              mcsTime = boost::posix_time::microsec_clock::universal_time();
              IntReadTrips( NULL, reqNodeOld, resNodeOld, delta_sql_old );
              delta_exec_old = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
              strOld = XMLTreeToText( ResDocOld );
              exec_old += delta_exec_old;
              exec_old_sql += delta_sql_old;
              mcsTime = boost::posix_time::microsec_clock::universal_time();
              IntReadTrips( NULL, reqNodeNew, resNodeNew, delta_sql_new );
              delta_exec_new = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
              strNew = XMLTreeToText( ResDocNew );
              exec_new += delta_exec_new;
              exec_new_sql += delta_sql_new;
            }
            else {
              step = 0;
              mcsTime = boost::posix_time::microsec_clock::universal_time();
              IntReadTrips( NULL, reqNodeNew, resNodeNew, delta_sql_new );
              delta_exec_new = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
              strNew = XMLTreeToText( ResDocNew );
              exec_new += delta_exec_new;
              exec_new_sql += delta_sql_new;
              mcsTime = boost::posix_time::microsec_clock::universal_time();
              IntReadTrips( NULL, reqNodeOld, resNodeOld, delta_sql_old );
              delta_exec_old = (boost::posix_time::microsec_clock::universal_time() - mcsTime).total_microseconds();
              strOld = XMLTreeToText( ResDocOld );
              exec_old += delta_exec_old;
              exec_old_sql += delta_sql_old;
            }
            if ( strNew != strOld ) {
              ProgTrace( TRACE5, "DIFF!!!" );
              f1 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
              f1 << strOld;
              f2 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
              f2 << strNew;
              file_size += strOld.size();
              file_size += strNew.size();
            }
          }
          catch(EXCEPTIONS::Exception &e) {
            f1 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
            f1 << "==========================EXCEPTIONS::Exception &e="<<e.what()<<endl;
            if ( !strOld.empty() ) {
              f1 << strOld;
            }
            f2 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
            f2 << "==========================EXCEPTIONS::Exception &e="<<e.what()<<endl;
            if ( !strNew.empty() ) {
              f2 << strNew;
            }
          }
          catch( ... ) {
            f1 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
            f1 << "==========================Unknown Error=============================="<<endl;
            f2 << "==========================user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<" "<<DateTimeToStr( day, ServerFormatDateTimeAsString ) << "=============================="<<endl;
            f2 << "==========================Unknown Error=============================="<<endl;
            xmlFreeDoc( ReqDocNew );
            xmlFreeDoc( ReqDocOld );
            xmlFreeDoc( ResDocNew );
            xmlFreeDoc( ResDocOld );
          }
          xmlFreeDoc( ReqDocNew );
          xmlFreeDoc( ReqDocOld );
          xmlFreeDoc( ResDocNew );
          xmlFreeDoc( ResDocOld );
          if ( delta_sql_new > delta_sql_old ) {
            f1<<"user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<",day="<<DateTimeToStr( day, ServerFormatDateTimeAsString )<<",step="<<step<<
                ", diff_exec_sql="<<((delta_sql_new-delta_sql_old)/1000)<<" ms, diff_exec="<<((delta_exec_new-delta_exec_old)/1000)<<" ms, delta_exec_new="<<delta_sql_new<<endl;
            f2<<"user_type="<<iuser_type<<",time_type="<<itime_type<<",icond="<<icond<<",day="<<DateTimeToStr( day, ServerFormatDateTimeAsString )<<",step="<<step<<
                ", diff_exec_sql="<<((delta_sql_new-delta_sql_old)/1000)<<" ms, diff_exec="<<((delta_exec_new-delta_exec_old)/1000)<<" ms, delta_exec_new="<<delta_sql_new<<endl;
            file_size += 100;
          }
        }
        if ( file_size > 10000 ) {
          file_size=0;
          if (f1.is_open()) f1.close();
          if (f2.is_open()) f2.close();
          file_count++;
          file_name=string("sopp_test_old.txt") + IntToString( file_count );
          f1.open(file_name.c_str());
          if (!f1.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
          file_name=string("sopp_test_new.txt") + IntToString( file_count );;
          f2.open(file_name.c_str());
          if (!f2.is_open()) throw EXCEPTIONS::Exception("Can't open file '%s'",file_name.c_str());
        }
      }
    }
  }
  f1<<"exec_new="<<(exec_new/1000)<<" ms    exec_new_sql="<<(exec_new_sql/1000)<<" ms"<<endl;
  f1<<"exec_old="<<(exec_old/1000)<<" ms    exec_old_sql="<<(exec_old_sql/1000)<<" ms"<<endl;
  f2<<"exec_new="<<(exec_new/1000)<<" ms    exec_new_sql="<<(exec_new_sql/1000)<<" ms"<<endl;
  f2<<"exec_old="<<(exec_old/1000)<<" ms    exec_old_sql="<<(exec_old_sql/1000)<<" ms"<<endl;
  if (f1.is_open()) f1.close();
  if (f2.is_open()) f2.close();
  return 0;
}

/*
void TZUpdate();

int tz_conversion(int, char **) {
    std::cout << "IANA timezone version: " << getTZDataVersion() << std::endl;

    TZUpdate();
    return 0;
}

#include "tz_test.h"

int test_conversion(int, char **) {
    std::cout << "IANA timezone version: " << getTZDataVersion() << std::endl;

    std::cout << "Creating points..." << std::endl;
    createPoints();
    std::cout << "Points created. Forming XML..." << std::endl;
    createXMLFromPoints();
    std::cout << "XML formed." << std::endl;

    return 0;
}
*/

#include "serverlib/xml_stuff.h" // для xml_decode_nodelist

int tst_vo(int, char**)
{
    string qry =
        "<?xml version='1.0' encoding='cp866'?> "
        "<term> "
        "  <query handle='0' id='print' ver='1' opr='DEN' screen='AIR.EXE' mode='STAND' lang='RU' term_id='158626489'> "
        "    <GetGRPPrintData> "
        "      <op_type>PRINT_BP</op_type> "
        "      <voucher/> "
        "      <grp_id>2591687</grp_id> "
        "      <pr_all>1</pr_all> "
        "      <dev_model>ML390</dev_model> "
        "      <fmt_type>EPSON</fmt_type> "
        "      <prnParams> "
        "        <pr_lat>0</pr_lat> "
        "        <encoding>CP866</encoding> "
        "        <offset>20</offset> "
        "        <top>0</top> "
        "      </prnParams> "
        "      <clientData> "
        "        <gate>31</gate> "
        "      </clientData> "
        "    </GetGRPPrintData> "
        "    <response/>" // сам дописал, туда будет засовываться ответ
        "  </query> "
        "</term> ";
    xmlDocPtr doc = TextToXMLTree(qry); // все данные в UTF
    xml_decode_nodelist(doc->children);
    xmlNodePtr rootNode=xmlDocGetRootElement(doc);
    TReqInfo *reqInfo = TReqInfo::Instance();
    reqInfo->Initialize("МОВ");
    XMLRequestCtxt *ctxt = getXmlCtxt();
    JxtInterfaceMng::Instance()->
        GetInterface("print")->
        OnEvent("GetGRPPrintData",  ctxt,
                rootNode->children->children,
                rootNode->children->children->next);
    LogTrace(TRACE5) << GetXMLDocText(rootNode->doc);
    return 1; // 0 - изменения коммитятся, 1 - rollback
}

string capt_code(const string &val, const string &suffix = "")
{
    string result = val;
    if(not suffix.empty())
        result += " " + suffix;
    result += ":";
    return result;
}

string tag_params(const string &date_format, const string &lang = "")
{
    ostringstream buf;
    if(not date_format.empty())
        buf << ",," << date_format;
    if(not lang.empty()) {
        if(buf.str().empty())
            buf << ",,," << lang;
        else
            buf << "," << lang;
    }
    ostringstream result;
    if(not buf.str().empty())
        result << "(" << buf.str() << ")";
    return result.str();
}

void move_pos(const string &code, int &xpos, int &ypos)
{
    if(code == "CITY_ARV_NAME")
    {
        ypos = 5;
        xpos = 80;
    }
    if(code == "NAME") {
        ypos = 5;
        xpos = 160;
    }
}

int prn_tags(int argc, char **argv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select op_type, code from prn_tag_props order by op_type, code";
    Qry.Execute();
    boost::optional<ofstream> prn_tags;
    int ypos = 20;
    string op_type;
    for(; not Qry.Eof; Qry.Next(), ypos += 5) {
        string tmp_op_type = Qry.FieldAsString("op_type");
        string code = Qry.FieldAsString("code");
        string date_format;
        if(
                code == "ACT" or
                code == "EST" or
                code == "SCD" or
                code == "TIME_PRINT"
          )
            date_format = "dd mmm yy hh:nn:ss";
        if(op_type != tmp_op_type) {
            op_type = tmp_op_type;
            prn_tags = boost::in_place("prn_tags_" + op_type);
        }
        prn_tags.get()
            << left << setw(20) << capt_code(code)
            << "'[<" << code << tag_params(date_format) << ">]'" << endl
            << left << setw(20) << capt_code(code, "RU")
            << "'[<" << code << tag_params(date_format, "R") << ">]'" << endl
            << left << setw(20) << capt_code(code, "EN")
            << "'[<" << code << tag_params(date_format, "E") << ">]'" << endl;
    }
    if(prn_tags) prn_tags.get().close();
    return 1;
}

/*
int prn_tags(int argc, char **argv)
{
    TQuery Qry(&OraSession);
    Qry.SQLText = "select code from prn_tag_props where op_type = 'PRINT_BP' order by code";
    Qry.Execute();
    ostringstream prn_tags;
    int ypos = 20;
    int xpos = 0;
    int file_idx = 0;
    for(; not Qry.Eof; Qry.Next(), ypos += 15) {
        string code = Qry.FieldAsString("code");
        string date_format;
        if(
                code == "ACT" or
                code == "EST" or
                code == "SCD" or
                code == "TIME_PRINT"
          )
            date_format = "dd mmm yy hh:nn:ss";
        ostringstream current_tag;
        current_tag
            << xpos << "," << ypos << ",0, " << setw(20) << left << capt_code(code) << "'[<" << code << tag_params(date_format) << ">]'" << endl
            << xpos << "," <<  ypos + 5 << ",0, " << setw(20) << left << capt_code(code, "RU") << "'[<" << code << tag_params(date_format, "R") << ">]'" << endl
            << xpos << "," <<  ypos + 10 << ",0, " << setw(20) << left << capt_code(code, "EN") << "'[<" << code << tag_params(date_format, "E") << ">]'" << endl;
        if(prn_tags.str().size() + current_tag.str().size() > 4000) {
            ostringstream file_name;
            file_name << "prn_tags." << file_idx++;
            ofstream of(file_name.str().c_str());
            of << prn_tags.str();
            prn_tags.str("");
        }
        prn_tags << current_tag.str();
        //        move_pos(code, xpos, ypos);
    }
    if(not prn_tags.str().empty()) {
        ostringstream file_name;
        file_name << "prn_tags." << file_idx++;
        ofstream of(file_name.str().c_str());
        of << prn_tags.str();
    }
    return 1;
}
*/

/*
int IsNosir(void)
{
  return LocalIsNosir==1;
}
*/

int tzdump(int argc,char **argv)
{
/*
  CREATE TABLE tzdump
  (
    version VARCHAR2(5) NOT NULL,
    utc_date DATE NOT NULL,
    local_date DATE NOT NULL,
    tz_region VARCHAR2(50) NOt NULL
  );
*/

  if(argc != 2) {
      cout << "Usage: " << argv[0] << " <version(e.g. 2018f)>" << endl;
      return 1;
  }

  string version=argv[1];

  TQuery Qry(&OraSession);

  set<string> regions;

  Qry.Clear();
  Qry.SQLText=
    "SELECT cities.tz_region "
    "FROM points, airps, cities "
    "WHERE points.airp=airps.code AND "
    "      airps.city=cities.code AND cities.tz_region IS NOT NULL "
    "UNION "
    "SELECT cities.tz_region "
    "FROM routes, airps, cities "
    "WHERE routes.airp=airps.code AND "
    "      airps.city=cities.code AND cities.tz_region IS NOT NULL ";
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
    regions.insert(Qry.FieldAsString("tz_region"));

  cout << "regions.size()=" << regions.size() << endl;

  Qry.Clear();
  Qry.SQLText="SELECT TRUNC(SYSDATE-180) AS min_date, "
              "       TRUNC(SYSDATE+60)  AS max_date "
              "FROM dual";
  Qry.Execute();
  TDateTime min_date=Qry.FieldAsDateTime("min_date");
  TDateTime max_date=Qry.FieldAsDateTime("max_date");

  Qry.Clear();
  Qry.SQLText="DELETE FROM tzdump WHERE version=:version";
  Qry.CreateVariable("version", otString, version);
  Qry.Execute();
  ASTRA::commit();

  Qry.Clear();
  Qry.SQLText="INSERT INTO tzdump(version, utc_date, local_date, tz_region) "
              "VALUES(:version, :utc_date, :local_date, :tz_region)";

  Qry.CreateVariable("version", otString, version);
  Qry.DeclareVariable("utc_date", otDate);
  Qry.DeclareVariable("local_date", otDate);
  Qry.DeclareVariable("tz_region", otString);

  for(const auto& region : regions)
  {
    cout << region << "...";
    Qry.SetVariable("tz_region", region);
    for(TDateTime d=min_date; d<=max_date; d+=1.0)
    {
      Qry.SetVariable("utc_date", d);
      Qry.SetVariable("local_date", UTCToLocal(d, region));
      Qry.Execute();
    }
    ASTRA::commit();
    cout << "ok" << endl;
  };

  return 0;
}

int tzdiff(int argc,char **argv)
{
  if(argc != 3) {
      cout << "Usage: " << argv[0] << " <version(e.g. 2016j)> <version(e.g. 2018i)>" << endl;
      return 1;
  }

  string version1=argv[1];
  string version2=argv[2];

  TQuery Qry(&OraSession);

  Qry.Clear();
  Qry.SQLText=
    "SELECT a.tz_region, a.utc_date, "
    "       (a.local_date-a.utc_date)*24 AS offset1, "
    "       (b.local_date-b.utc_date)*24 AS offset2 "
    "FROM tzdump a, tzdump b "
    "WHERE a.utc_date=b.utc_date AND "
    "      a.tz_region=b.tz_region AND "
    "      a.version=:version1 AND b.version=:version2 AND a.local_date<>b.local_date "
    "ORDER BY a.tz_region, a.utc_date";
  Qry.CreateVariable("version1", otString, version1);
  Qry.CreateVariable("version2", otString, version2);
  Qry.Execute();

  TQuery AirpsQry(&OraSession);
  AirpsQry.Clear();
  AirpsQry.SQLText=
      "SELECT LISTAGG(airp, ', ') WITHIN GROUP (ORDER BY airp) AS airps "
      "FROM "
      "(SELECT points.airp "
      " FROM points, airps, cities "
      " WHERE points.airp=airps.code AND "
      "       airps.city=cities.code AND cities.tz_region=:tz_region "
      " UNION "
      " SELECT routes.airp "
      " FROM routes, airps, cities "
      " WHERE routes.airp=airps.code AND "
      "       airps.city=cities.code AND cities.tz_region=:tz_region) ";

  AirpsQry.DeclareVariable("tz_region", otString);

  struct Row
  {
    string tz_region;
    TDateTime utc_date;
    double offset1;
    double offset2;

    bool empty() const { return tz_region.empty(); }
  };

  Row prior;

  while(true)
  {
    Row curr;
    if (!Qry.Eof)
    {
      curr.tz_region=Qry.FieldAsString("tz_region");
      curr.utc_date=Qry.FieldAsDateTime("utc_date");
      curr.offset1=Qry.FieldAsFloat("offset1");
      curr.offset2=Qry.FieldAsFloat("offset2");
    }

    if (prior.empty() && curr.empty())
      cout << "No diffs found :)" << endl;
    else
      if (prior.tz_region!=curr.tz_region ||
          prior.utc_date!=curr.utc_date-1.0 ||
          prior.offset1!=curr.offset1 ||
          prior.offset2!=curr.offset2)
      {
        if (!prior.empty())
        {
          cout << "-" << DateTimeToStr(prior.utc_date, "dd.mm.yy") << ": " << endl;
          cout << "  " << version1 << ": GMT" << std::showpos << fixed << setprecision(2) << prior.offset1 << endl;
          cout << "  " << version2 << ": GMT" << std::showpos << fixed << setprecision(2) << prior.offset2 << endl;
          cout << "  airports: ";
          AirpsQry.SetVariable("tz_region", prior.tz_region);
          AirpsQry.Execute();
          if (!AirpsQry.Eof) cout << AirpsQry.FieldAsString("airps");
          cout << endl;
        }

        if (!curr.empty())
        {
          cout << curr.tz_region << " " << DateTimeToStr(curr.utc_date, "dd.mm.yy");
        }
      }

    if (Qry.Eof) break;

    Qry.Next();

    prior=curr;
  }

  return 0;
}
