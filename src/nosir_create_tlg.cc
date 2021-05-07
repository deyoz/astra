#include "nosir_create_tlg.h"
#include "date_time.h"
#include "qrys.h"
#include "stl_utils.h"
#include "typeb_utils.h"
#include "telegram.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace ASTRA;

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

        DB::TCachedQuery TlgQry(
                    PgOra::getROSession("TLG_OUT"),
                    "SELECT * FROM tlg_out WHERE id=:id ORDER BY num",
                    QParams() << QParam("id", otInteger),
                    STDLOG);

        boost::posix_time::ptime time1 = DateTimeToBoost(time_create);
        boost::posix_time::ptime time2 = time1 + boost::posix_time::hours(interval_hours);
        DB::TCachedQuery Qry(
                    PgOra::getROSession("TLG_OUT"),
                    "SELECT distinct point_id, type FROM tlg_out WHERE "
                    "   time_create > :time1 AND "
                    "   time_create <= :time2 AND "
                    "   type NOT IN ('BTM') "
                    "ORDER BY point_id, type ",
                    QParams()
                    << QParam("time1", otDate, BoostToDateTime(time1))
                    << QParam("time2", otDate, BoostToDateTime(time2)),
                    STDLOG);

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
