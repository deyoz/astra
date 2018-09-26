#include "franchise.h"
#include "qrys.h"
#include "astra_misc.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;
using namespace BASIC::date_time;

namespace Franchise {
    bool TProp::get(int point_id, const string &tlg_type)
    {
        clear();
        if(PropTypesTlg().decode(tlg_type) == TPropType::Unknown)
            return false;
        else
            return get(point_id, PropTypesTlg().decode(tlg_type));
    }

    bool TProp::get(const TTripInfo &info, TPropType::Enum prop, bool is_local_scd_out)
    {
        clear();
        TDateTime scd_local = is_local_scd_out ? info.scd_out : UTCToLocal(info.scd_out, AirpTZRegion(info.airp));
        oper.airline = info.airline;
        oper.flt_no = info.flt_no;
        oper.suffix = info.suffix;
        TCachedQuery Qry(
                "select * from franchise_sets where "
                "   airp_dep = :airp and "
                "   airline = :airline and "
                "   flt_no = :flt_no and "
                "   nvl(suffix, ' ') = nvl(:suffix, ' ') and "
                "   :scd_out >= first_date and "
                "   (last_date is null or :scd_out < last_date) and "
                "   pr_denial = 0 ",
                QParams()
                << QParam("airp", otString, info.airp)
                << QParam("airline", otString, info.airline)
                << QParam("flt_no", otInteger, info.flt_no)
                << QParam("suffix", otString, info.suffix)
                << QParam("scd_out", otDate, scd_local)
                );
        Qry.get().Execute();
        if(not Qry.get().Eof) {
            franchisee.airline = Qry.get().FieldAsString("airline_franchisee");
            franchisee.flt_no = Qry.get().FieldAsInteger("flt_no_franchisee");
            franchisee.suffix = Qry.get().FieldAsString("suffix_franchisee");
            string prop_name = PropTypes().encode(prop);
            if(Qry.get().FieldIsNULL(prop_name))
                val = pvEmpty;
            else {
                val = (Qry.get().FieldAsInteger(prop_name) != 0 ? pvYes : pvNo);
            }
        }
        return val != pvUnknown;
    }

    bool TProp::get(int point_id, TPropType::Enum prop)
    {
        clear();
        TTripInfo info;
        if(info.getByPointId(point_id))
            return get(info, prop, false);
        return val != pvUnknown;
    }

    bool TProp::get( const TTripInfo &info, TPropType::Enum prop ) {
      clear();

      TDateTime scd_local = UTCToLocal(info.scd_out, AirpTZRegion(info.airp));

      franchisee.airline = info.airline;
      franchisee.flt_no = info.flt_no;
      franchisee.suffix = info.suffix;
      ProgTrace( TRACE5, "airline=%s, flt_no=%d", franchisee.airline.c_str(),franchisee.flt_no );
      TCachedQuery Qry(
          "select * from franchise_sets where "
          "   airp_dep = :airp and "
          "   airline_franchisee = :airline and "
          "   flt_no_franchisee = :flt_no and "
          "   nvl(suffix_franchisee, ' ') = nvl(:suffix, ' ') and "
          "   :scd_out >= first_date and "
          "   (last_date is null or :scd_out < last_date) and "
          "   pr_denial = 0 ",
                  QParams()
                  << QParam("airp", otString, info.airp)
                  << QParam("airline", otString, info.airline)
                  << QParam("flt_no", otInteger, info.flt_no)
                  << QParam("suffix", otString, info.suffix)
                  << QParam("scd_out", otDate, scd_local)
                  );
          Qry.get().Execute();
          tst();
          if(not Qry.get().Eof) {
              tst();
              oper.airline = Qry.get().FieldAsString("airline");
              oper.flt_no = Qry.get().FieldAsInteger("flt_no");
              oper.suffix = Qry.get().FieldAsString("suffix");
              string prop_name = PropTypes().encode(prop);
              if(Qry.get().FieldIsNULL(prop_name)) {
                  val = pvEmpty;
                  tst();
              }
              else {
                  val = (Qry.get().FieldAsInteger(prop_name) != 0 ? pvYes : pvNo);
                  tst();
              }
          }
      ProgTrace( TRACE5, "val=%d", (int)val );
      return val != pvUnknown;
    }

    const TPropTypes &PropTypes()
    {
        static TPropTypes propTypes;
        return propTypes;
    }

    const TPropTypesTlg &PropTypesTlg()
    {
        static TPropTypesTlg propTypesTlg;
        return propTypesTlg;
    }

    void TProp::clear()
    {
        oper.clear();
        franchisee.clear();
        val = pvUnknown;
    }

    void TFlight::clear()
    {
        airline.clear();
        flt_no = NoExists;
        suffix.clear();
    }
}
