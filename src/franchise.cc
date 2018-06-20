#include "franchise.h"
#include "qrys.h"
#include "astra_misc.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace ASTRA;
using namespace std;

namespace Franchise {
    bool TProp::get(int point_id, const string &tlg_type)
    {
        clear();
        if(PropTypesTlg().decode(tlg_type) == TPropType::Unknown)
            return false;
        else
            return get(point_id, PropTypesTlg().decode(tlg_type));
    }

    bool TProp::get(int point_id, TPropType::Enum prop)
    {
        clear();
        TTripInfo info;
        if(
                info.getByPointId(point_id) and
                info.trip_type == "ä" and
                ((const TTripTypesRow&)base_tables.get("trip_types").get_row( "code", info.trip_type, true )).pr_reg!=0
          ) {
            oper.airline = info.airline;
            oper.flt_no = info.flt_no;
            oper.suffix = info.suffix;
            TCachedQuery Qry(
                    "select * from franchise_sets where "
                    "   airline = :airline and "
                    "   flt_no = :flt_no and "
                    "   nvl(suffix, ' ') = nvl(:suffix, ' ') and "
                    "   :scd_out >= first_date and "
                    "   (last_date is null or :scd_out < last_date) and "
                    "   pr_denial = 0 ",
                    QParams()
                    << QParam("airline", otString, info.airline)
                    << QParam("flt_no", otInteger, info.flt_no)
                    << QParam("suffix", otString, info.suffix)
                    << QParam("scd_out", otDate, info.scd_out)
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
        }
        return val != pvUnknown;
    }
    bool TProp::get( const TTripInfo &info, TPropType::Enum prop ) {
      clear();
      franchisee.airline = info.airline;
      franchisee.flt_no = info.flt_no;
      franchisee.suffix = info.suffix;
      TCachedQuery Qry(
          "select * from franchise_sets where "
          "   airline_franchisee = :airline and "
          "   flt_no_franchisee = :flt_no and "
          "   nvl(suffix_franchisee, ' ') = nvl(:suffix, ' ') and "
          "   :scd_out >= first_date and "
          "   (last_date is null or :scd_out < last_date) and "
          "   pr_denial = 0 ",
                  QParams()
                  << QParam("airline", otString, info.airline)
                  << QParam("flt_no", otInteger, info.flt_no)
                  << QParam("suffix", otString, info.suffix)
                  << QParam("scd_out", otDate, info.scd_out)
                  );
          Qry.get().Execute();
          if(not Qry.get().Eof) {
              oper.airline = Qry.get().FieldAsString("airline");
              oper.flt_no = Qry.get().FieldAsInteger("flt_no");
              oper.suffix = Qry.get().FieldAsString("suffix");
              string prop_name = PropTypes().encode(prop);
              if(Qry.get().FieldIsNULL(prop_name))
                  val = pvEmpty;
              else {
                  val = (Qry.get().FieldAsInteger(prop_name) != 0 ? pvYes : pvNo);
              }
          }
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
