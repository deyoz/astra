#include "rbd.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;

TFlightRbd::TFlightRbd(const TTripInfo& flt)
{
  clear();

  boost::optional<int> priority=getMaxRbdPriority(flt);
  if (!priority)
  {
    ProgError(STDLOG,"Class group not found (airline=%s, airp=%s)", flt.airline.c_str(), flt.airp.c_str());
    return;
  }

  TQuery Qry(&OraSession);
  for(int pass=0; pass<2; pass++)
  {
    Qry.Clear();
    pass==0?
      Qry.SQLText=
        "SELECT cls_grp.id AS subclass_group, subcls_grp.subclass AS fare_class "
        "FROM cls_grp, subcls_grp "
        "WHERE cls_grp.id=subcls_grp.id AND "
        "      (airline IS NULL OR airline=:airline) AND "
        "      (airp IS NULL OR airp=:airp) AND "
        "      DECODE(airline,NULL,0,4)+ "
        "      DECODE(airp,NULL,0,2) = :priority AND "
        "      cls_grp.pr_del=0 "
        "ORDER BY subclass_group":
      Qry.SQLText=
        "SELECT cls_grp.id AS subclass_group, cls_grp.class AS fare_class "
        "FROM cls_grp "
        "WHERE (airline IS NULL OR airline=:airline) AND "
        "      (airp IS NULL OR airp=:airp) AND "
        "      DECODE(airline,NULL,0,4)+ "
        "      DECODE(airp,NULL,0,2) = :priority AND "
        "      cls_grp.class IS NOT NULL AND cls_grp.pr_del=0 "
        "ORDER BY subclass_group";
    Qry.CreateVariable("airline", otString, flt.airline);
    Qry.CreateVariable("airp", otString, flt.airp);
    Qry.CreateVariable("priority", otInteger, priority.get());
    Qry.Execute();
    for(; !Qry.Eof; Qry.Next())
    {
      TRbdItem<TSubclassGroup> item;
      item.fromDB(Qry);
      if (!(pass==0?fare_classes:base_classes).emplace(item, item).second)
        ProgTrace(TRACE5, pass==0?"More than one class group found (airline=%s, airp=%s, subclass=%s)":
                                  "More than one class group found (airline=%s, airp=%s, class=%s)",
                          flt.airline.c_str(),
                          flt.airp.c_str(),
                          item.fare_class.c_str());
    }
  }
}

boost::optional<int> TFlightRbd::getMaxRbdPriority(const TTripInfo& flt) const
{
  TQuery Qry(&OraSession);
  Qry.Clear();
  Qry.SQLText=
    "SELECT MAX(DECODE(airline,NULL,0,4)+ "
    "           DECODE(airp,NULL,0,2)) AS priority "
    "FROM cls_grp "
    "WHERE (airline IS NULL OR airline=:airline) AND "
    "      (airp IS NULL OR airp=:airp) AND "
    "      pr_del=0";
  Qry.CreateVariable("airline", otString, flt.airline);
  Qry.CreateVariable("airp", otString, flt.airp);
  Qry.Execute();
  if (Qry.Eof || Qry.FieldIsNULL("priority")) return boost::none;

  return Qry.FieldAsInteger("priority");
}

boost::optional<TSubclassGroup> TFlightRbd::getSubclassGroup(const std::string& subcls, const std::string& cls) const
{
  std::map<TRbdKey, TRbdItem<TSubclassGroup> >::const_iterator i=fare_classes.find(TRbdKey(subcls));
  if (i!=fare_classes.end()) return i->second.compartment;
  i=base_classes.find(TRbdKey(cls));
  if (i!=base_classes.end()) return i->second.compartment;
  return boost::none;
}

void TFlightRbd::dump(const std::string& whence) const
{
  for(int pass=0; pass<2; pass++)
  {
    LogTrace(TRACE5) << whence << ": " << (pass==0?"fare_classes dump":"base_classes dump");
    for(const auto& i : (pass==0?fare_classes:base_classes))
    {
      LogTrace(TRACE5) << whence << ": "
                       << setw(3) << i.second.fare_class
                       << setw(10) << (i.second.compartment?IntToString(i.second.compartment.get().value()):"none");
    }
  }
}

#include "rbd.h"

int rbd_test(int argc,char **argv)
{
  {
    TTripInfo flt;
    flt.airline="ž’";
    flt.airp="„Œ„";

    TFlightRbd rbds(flt);
    rbds.dump("rbd_test");
  }
  {
    TTripInfo flt;
    flt.airline="Ž";
    flt.airp="„Œ„";

    TFlightRbd rbds(flt);
    rbds.dump("rbd_test");
  }
  {
    TTripInfo flt;
    flt.airline="“";
    flt.airp="„Œ„";

    TFlightRbd rbds(flt);
    rbds.dump("rbd_test");
  }
  {
    TTripInfo flt;
    flt.airline="2";
    flt.airp="„Œ„";

    TFlightRbd rbds(flt);
    rbds.dump("rbd_test");
  }
  {
    TTripInfo flt;
    flt.airline="‹€";
    flt.airp="„Œ„";

    TFlightRbd rbds(flt);
    rbds.dump("rbd_test");
  }

  return 0;
}

