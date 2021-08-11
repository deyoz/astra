#include "nosir_create_tlg.h"
#include "date_time.h"
#include "qrys.h"
#include "stl_utils.h"
#include "typeb_utils.h"
#include "telegram.h"
#include "obrnosir.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace AstraLocale;

void create_tlg_usage(string name, string what)
{
    cout 
        << "Error: " << what << endl
        << "Usage: " << name << " <yymmddhhnn> <hours>" << endl;
    cout
        << "Example:" << endl
        << "  " << name << " 1906011200 1" << endl;

}

int nosir_create_tlg(int argc, char **argv)
{
    try {
        if(argc != 3)
            throw Exception("wrong params");

        TDateTime time_create;
        if ( StrToDateTime( argv[1], "yymmddhhnn", time_create ) == EOF )
            throw Exception("wrong date format");
        int interval_hours = ToInt(argv[2]);

        TCachedQuery TlgQry("SELECT * FROM tlg_out WHERE id=:id ORDER BY num",
                QParams() << QParam("id", otInteger));

        TCachedQuery Qry(
                "select distinct point_id, type from tlg_out where "
                "   time_create > :time_create and "
                "   time_create <= :time_create + :interval_hours/24 and "
                "   type not in ('BTM') "
                "order by point_id, type ",
                QParams()
                << QParam("time_create", otDate, time_create)
                << QParam("interval_hours", otInteger, interval_hours));

        Qry.get().Execute();
        boost::optional<ofstream> file;
        for(; not Qry.get().Eof; Qry.get().Next()) {
            string tlg_type = Qry.get().FieldAsString("type");
            TypeB::TCreateInfo info(tlg_type, TypeB::TCreatePoint());
            info.point_id = Qry.get().FieldAsInteger("point_id");
            TTypeBTypesRow tlgTypeInfo;
            int tlg_id = NoExists;
            try {
                tlg_id = TelegramInterface::create_tlg(info, ASTRA::NoExists, tlgTypeInfo, true);
            } catch(const Exception &E) {
                LogTrace(TRACE5)
                    << "create_tlg failed for point_id = '" << info.point_id
                    << "', tlg_type = '" << tlg_type << "': " << E.what();
                continue;
            }

            TlgQry.get().SetVariable("id", tlg_id);
            TlgQry.get().Execute();
            for(;!TlgQry.get().Eof;TlgQry.get().Next())
            {
                TTlgOutPartInfo tlg;
                tlg.fromDB(TlgQry.get());
                if(not file) file = boost::in_place("tlg_out_created", std::ios::binary|std::ios::trunc);
                file.get() << tlg.heading + tlg.body + tlg.ending;
            }
            ASTRA::rollback();
        }
        if(file) file.get().close();
    } catch(Exception &E) {
        create_tlg_usage(argv[0], E.what());
        return 1;
    }
    return 1;
}

static void nosirDumpTypeBOutUsage(const string& command, const string& what)
{
  if (!what.empty())
  {
    cout << "error! " << what << endl << endl;
  }

  cout << "usage: " << command << " <dir> <tlgType> <firstFlightLocalDate> <lastFlightLocalDate> <airline>" << endl;
  cout << "       date format: yyyymmdd" << endl;
  cout << "example: " << command << " PRL_R3 PRL 20210601 20210630 R3" << endl;
}

static std::vector<TTripInfo> getFlights(const AirlineCode_t& airline,
                                         const TDateTime firstLocalDateTime,
                                         const TDateTime lastLocalDateTime)
{
  std::vector<TTripInfo> result;

  DB::TQuery Qry(PgOra::getROSession("POINTS"), STDLOG);

  Qry.SQLText="SELECT " + TTripInfo::selectedFields() + " FROM points "
              "WHERE airline=:airline AND scd_out>=:min_scd_out AND scd_out<:max_scd_out AND pr_del>=0";

  Qry.CreateVariable("airline", otString, airline.get());
  Qry.CreateVariable("min_scd_out", otDate, firstLocalDateTime-2.0);
  Qry.CreateVariable("max_scd_out", otDate, lastLocalDateTime+3.0);
  Qry.Execute();
  for(; !Qry.Eof; Qry.Next())
  {
    TTripInfo flt(Qry);
    TDateTime localScdOut=UTCToLocal(flt.scd_out, AirpTZRegion(flt.airp));
    if (localScdOut>=firstLocalDateTime && localScdOut<lastLocalDateTime)
      result.push_back(flt);
  }

  return result;
}

static void dumpTypeBOut(const string& dir,
                         const string& tlgType,
                         const std::vector<TTripInfo>& flts)
{
  DB::TQuery Qry(PgOra::getROSession("TLG_OUT"), STDLOG);
  Qry.SQLText="SELECT id, num, addr, origin, heading, body, ending FROM tlg_out WHERE point_id=:point_id AND type=:type";
  Qry.DeclareVariable("point_id", otInteger);
  Qry.CreateVariable("type", otString, tlgType);

  cout << "total " << flts.size() << " flights" << endl;

  OutputLang lang_en(LANG_EN);
  int processed=0;
  for(const TTripInfo& flt : flts)
  {
    nosir_wait(processed++, false, 10, 0);

    TDateTime localScdOut=UTCToLocal(flt.scd_out, AirpTZRegion(flt.airp));
    string filenamePrefix = "./" + dir + "/" + tlgType
                            + "_" + airlineToPrefferedCode(flt.airline, lang_en) + flt.flight_number(lang_en)
                            + "_" + DateTimeToStr(localScdOut, "ddmmm", 1)
                            + "_" + airpToPrefferedCode(flt.airp, lang_en);

    Qry.SetVariable("point_id", flt.point_id);
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next())
    {
      string filename = filenamePrefix
                        + "." + IntToString(Qry.FieldAsInteger("id"))
                        + ".PART" + IntToString(Qry.FieldAsInteger("num"))
                        + ".tlg";

      ofstream f(filename, ios_base::trunc|ios_base::out);
      f << Qry.FieldAsString("addr")
        << Qry.FieldAsString("origin")
        << Qry.FieldAsString("heading")
        << Qry.FieldAsString("body")
        << Qry.FieldAsString("ending");
    }
  }
}

int nosirDumpTypeBOut(int argc, char **argv)
{
  try
  {
    if (argc<6) throw Exception("wrong params");
    string dir(argv[1]);
    string tlgType(argv[2]);

    TElemFmt fmt;
    string airline(ElemToElemId(etAirline, argv[5], fmt));
    if (airline.empty()) throw Exception("wrong airline: %s", argv[5]);

    TDateTime firstLocalDate=ASTRA::NoExists;
    TDateTime lastLocalDate=ASTRA::NoExists;

    if(StrToDateTime(argv[3], "yyyymmdd", firstLocalDate) == EOF)
      throw Exception("wrong firstFlightLocalDate: %s", argv[3]);
    if(StrToDateTime(argv[4], "yyyymmdd", lastLocalDate) == EOF)
      throw Exception("wrong lastFlightLocalDate: %s", argv[4]);
    if (firstLocalDate>lastLocalDate)
      throw Exception("wrong date range: [%s, %s]", argv[3], argv[4]);


    std::vector<TTripInfo> flts=getFlights(AirlineCode_t(airline),
                                           firstLocalDate,
                                           lastLocalDate+1.0);
    dumpTypeBOut(dir, tlgType, flts);
  }
  catch(const std::exception &e)
  {
    nosirDumpTypeBOutUsage(argv[0], e.what());
  }

  return 1;
}

void nosirDumpTypeBOutUsage(const char* name)
{
  nosirDumpTypeBOutUsage(name, "");
}

