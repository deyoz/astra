#include "ldm_parser.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "qrys.h"
#include "sopp.h"
#include "flt_settings.h"

#define NICKNAME "DEN"
#include <serverlib/slogger.h>

using namespace std;
using namespace ASTRA;

namespace TypeB
{
    namespace LDMParser
    {
        string TLDMContent::toString()
        {
            ostringstream result;
            result << "bort: '" << bort << "'; cockpit: " << cockpit << "; cabin: " << cabin;
            return result.str();
        }

        void TLDMContent::clear()
        {
            bort.clear();
            cockpit = NoExists;
            cabin = NoExists;
        }

        void ParseLDMContent(TTlgPartInfo body, TUCMHeadingInfo& info, TLDMContent& con, TMemoryManager &mem) {
            con.clear();
            // Пробег по телу здесь не нужен, т.к. вся инфа была в заголовке вместе с номером рейса
            vector<string>tokens;
            boost::split(tokens, info.flt_info.src, boost::is_any_of("."));
            if(tokens.size() != 4)
                throw ETlgError("wrong LDM format");
            con.bort = tokens[1];

            boost::match_results<std::string::const_iterator> results;
            static const boost::regex regex_bort("^(" + regex::m + "{2,10}" + ")$");
            if(not boost::regex_match(con.bort, results, regex_bort))
                throw ETlgError(tlgeNotMonitorNotAlarm, "Wrong bort: '" + con.bort + "'");

            static const boost::regex crew1("^(\\d)/(\\d{1,2})$");
            static const boost::regex crew2("^(\\d)/(\\d{1,2})/(\\d{1,2})$");

            string crew = tokens[3];
            if(boost::regex_match(crew, results, crew1)) {
                con.cockpit = ToInt(results[1]);
                con.cabin = ToInt(results[2]);
            } else if(boost::regex_match(crew, results, crew2)) {
                con.cockpit = ToInt(results[1]);
                con.cabin = ToInt(results[2]) + ToInt(results[3]);
            } else
                throw ETlgError(tlgeNotMonitorNotAlarm, "Wrong crew: " + crew);
        }

        void SaveLDMContent(int tlg_id, TUCMHeadingInfo& info, TLDMContent& con) {
            int point_id_tlg = SaveFlt(tlg_id,info.flt_info.toFltInfo(),btFirstSeg);
            DB::TCachedQuery Qry(
                  PgOra::getROSession("TLG_BINDING"),
                  "SELECT point_id_spp "
                  "FROM tlg_binding "
                  "WHERE point_id_tlg = :point_id_tlg",
                  QParams() << QParam("point_id_tlg", otInteger, point_id_tlg),
                  STDLOG);
            Qry.get().Execute();
            for(; not Qry.get().Eof; Qry.get().Next()) {
                TTripInfo flt;
                flt.getByPointId(Qry.get().FieldAsInteger("point_id_spp"));
                if(GetTripSets(tsProcessInboundLDM, flt)) {
                    ChangeBortFromLDM(con.bort, Qry.get().FieldAsInteger(0));
                    UpdateCrew(Qry.get().FieldAsInteger(0), "", con.cockpit, con.cabin, ownerLDM);
                }
            }
        }
    }
}
