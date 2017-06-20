#include "oralib.h"
#include "exceptions.h"
#include "stat.h"
#include "qrys.h"
#include "passenger.h"
#include "obrnosir.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/slogger.h"

using namespace BASIC::date_time;
using namespace std;
using namespace STAT;

int stat_belgorod(int argc, char **argv)
{
  TReqInfo::Instance()->Initialize("МОВ");

  const string belgorod_airp="БЕД";

  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText =
    "SELECT TO_DATE('01.01.2016','DD.MM.YYYY') AS min_date, "
    "       TO_DATE('01.04.2017','DD.MM.YYYY') AS max_date "
    "FROM dual";
  Qry.Execute();
  TDateTime min_date=Qry.FieldAsDateTime("min_date");
  TDateTime max_date=Qry.FieldAsDateTime("max_date");

  ostringstream fname_arv;
  fname_arv << "belgorod_frgn_arr_"
            << DateTimeToStr(min_date, "yyyy_mm_dd") << "-" << DateTimeToStr(max_date, "yyyy_mm_dd")
            << ".txt";
  ostringstream fname_dep;
  fname_dep << "belgorod_frgn_dep_"
            << DateTimeToStr(min_date, "yyyy_mm_dd") << "-" << DateTimeToStr(max_date, "yyyy_mm_dd")
            << ".txt";

  const string delim="\t";
  const string endl="\r\n";

  set<string> moscow_airps;
  Qry.Clear();
  Qry.SQLText =
    "SELECT code FROM airps WHERE city=:city AND pr_del=0";
  Qry.CreateVariable("city", otString, "МОВ");
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
    moscow_airps.insert(Qry.FieldAsString("code"));

  TCachedQuery PointsQry(
    "SELECT pax.*, pax_grp.airp_dep, pax_grp.airp_arv, points.point_id "
    "FROM points, pax_grp, pax, pax_doc "
    "WHERE points.point_id=pax_grp.point_dep AND "
    "      pax_grp.grp_id=pax.grp_id AND "
    "      pax.pax_id=pax_doc.pax_id AND "
    "      points.move_id=:move_id AND "
    "      points.pr_del>=0 AND "
    "      pax_grp.status NOT IN ('E') AND (pax_grp.airp_dep=:airp OR pax_grp.airp_arv=:airp) AND "
    "      pax.refuse IS NULL AND pax_doc.issue_country<>'RUS' ",
    QParams() << QParam("move_id", otInteger)
              << QParam("airp", otString, belgorod_airp));

  TCachedQuery ArxPointsQry(
    "SELECT arx_pax.*, NULL AS crew_type, arx_pax_grp.airp_dep, arx_pax_grp.airp_arv, arx_points.point_id "
    "FROM arx_points, arx_pax_grp, arx_pax, arx_pax_doc "
    "WHERE arx_points.part_key=arx_pax_grp.part_key AND "
    "      arx_points.point_id=arx_pax_grp.point_dep AND "
    "      arx_pax_grp.part_key=arx_pax.part_key AND "
    "      arx_pax_grp.grp_id=arx_pax.grp_id AND "
    "      arx_pax.part_key=arx_pax_doc.part_key AND "
    "      arx_pax.pax_id=arx_pax_doc.pax_id AND "
    "      arx_points.part_key=:part_key AND "
    "      arx_points.move_id=:move_id AND "
    "      arx_points.pr_del>=0 AND "
    "      arx_pax_grp.status NOT IN ('E') AND (arx_pax_grp.airp_dep=:airp OR arx_pax_grp.airp_arv=:airp) AND "
    "      arx_pax.refuse IS NULL AND arx_pax_doc.issue_country<>'RUS' ",
    QParams() << QParam("part_key", otDate)
              << QParam("move_id", otInteger)
              << QParam("airp", otString, belgorod_airp));


  std::ofstream farv(fname_arv.str().c_str());
  std::ofstream fdep(fname_dep.str().c_str());
  try
  {
    if(!farv.is_open())
      throw EXCEPTIONS::Exception("Can't open file %s", fname_arv.str().c_str());

    if(!fdep.is_open())
      throw EXCEPTIONS::Exception("Can't open file %s", fname_dep.str().c_str());

    for(int pass=0; pass<2; pass++)
    {
      (pass==0?farv:fdep)
         << "ФИО" << delim
         << "Дата рождения" << delim
         << "Номер паспорта" << delim
         << "Код страны, выдавшей паспорт" << delim
         << "Направление" << delim
         << "Номер рейса" << delim
         << "Дата вылета рейса (UTC)" << endl;
    }

    int processed=0;
    for(; min_date<max_date; min_date+=1.0)
    {
      TMoveIds move_ids;
      move_ids.get_for_airp(min_date, min_date+1.0, belgorod_airp);
      printf("%s: move_ids.size()=%zu\n", DateTimeToStr(min_date, "dd.mm.yyyy").c_str(), move_ids.size());
      for(TMoveIds::const_iterator i=move_ids.begin(); i!=move_ids.end(); ++i)
      {
        map< pair<TDateTime, int>, TTripInfo > flights;

        TDateTime part_key=i->first;
        int move_id=i->second;
        TQuery &PQry=(part_key==ASTRA::NoExists?PointsQry.get():ArxPointsQry.get());
        if (part_key!=ASTRA::NoExists)
          PQry.SetVariable("part_key", part_key);
        PQry.SetVariable("move_id", move_id);
        PQry.Execute();
        for(;!PQry.Eof;PQry.Next())
        {
          CheckIn::TSimplePaxItem pax;
          pax.fromDB(PQry);
          CheckIn::TPaxDocItem doc;
          if (!CheckIn::LoadPaxDoc(part_key, pax.id, doc)) continue;

          string airp_dep=PQry.FieldAsString("airp_dep");
          string airp_arv=PQry.FieldAsString("airp_arv");
          int point_id=PQry.FieldAsInteger("point_id");

          map< pair<TDateTime, int>, TTripInfo >::iterator flt=flights.insert(make_pair(make_pair(part_key, point_id), TTripInfo())).first;
          if (flt==flights.end())
            throw EXCEPTIONS::Exception("%s: strange situation flt==flights.end()!", __FUNCTION__);

          if (flt->second.airline.empty())
            flt->second.getByPointId(part_key, point_id);

          for(int pass=0; pass<2; pass++)
          {
            nosir_wait(processed, false, 20, 1);
            if ((pass==0 && (airp_arv!=belgorod_airp || moscow_airps.find(airp_dep)==moscow_airps.end())) ||
                (pass!=0 && airp_dep!=belgorod_airp)) continue;

            std::ofstream &f=(pass==0?farv:fdep);
            //ФИО
            if (!doc.surname.empty() && !doc.first_name.empty())
              f << doc.full_name();
            else
              f << pax.full_name();
            f << delim;
            //Дата рождения
            if (doc.birth_date!=ASTRA::NoExists)
              f << DateTimeToStr(doc.birth_date, "dd.mm.yyyy");
            f << delim;
            //Номер паспорта
            f << doc.no << delim;
            //Код страны, выдавшей паспорт
            f << doc.issue_country << delim;
            //Направление
            f << (pass==0?airp_dep:airp_arv) << delim;
            //Номер рейса
            f << flt->second.flight_view(ecNone, false, false) << delim;
            //Дата вылета рейса (UTC)
            if (flt->second.scd_out!=ASTRA::NoExists)
              f << DateTimeToStr(flt->second.scd_out, "dd.mm.yyyy");
            f << endl;
          };
          processed++;
        }
      }
    }
    if (farv.is_open()) farv.close();
    if (fdep.is_open()) fdep.close();
  }
  catch(...)
  {
    if (farv.is_open()) farv.close();
    if (fdep.is_open()) fdep.close();
    throw;
  }

  return 1;
}

