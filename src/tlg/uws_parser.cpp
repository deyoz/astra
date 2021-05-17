#include "uws_parser.h"
#include <boost/regex.hpp>
#include "sopp.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace ASTRA;

namespace TypeB
{

    namespace UWSParser
    {
        enum TUWSElem {
            uwsFlightElement,
            uwsData
        };

        void ParseUWSContent(TTlgPartInfo body, TAHMHeadingInfo& info, TUWSContent& con, TMemoryManager &mem) {
            static const boost::regex uws_line("^-.*/(\\d{1,5})" + regex::a + "/(" + regex::a + ").*$");
            static const boost::regex uws_line_airp("^-.*/" + regex::airp + "/(\\d{1,5})" + regex::a + "/(" + regex::a + ").*$");
            static const boost::regex uws_line_bulk("^-" + regex::airp  + "/" + "(\\d{1,4})" + regex::a + "/\\d{1,3}/(" + regex::a + ").*$");

            con.Clear();
            TTlgParser tlg;
            const char *line_p=body.p;
            TUWSElem e = uwsFlightElement;
            try
            {
                do {
                    tlg.GetToEOLLexeme(line_p);
                    if(not *tlg.lex)
                        throw ETlgError("blank line not allowed");
                    switch(e) {
                        case uwsFlightElement:
                            {
                                TTlgPartInfo tmp_body;
                                tmp_body.p = line_p;
                                ParseAHMFltInfo(tmp_body,info,info.flt,info.bind_type);
                                e = uwsData;
                            }
                            break;
                        case uwsData:
                            {
                                const string &line = tlg.lex;
                                boost::match_results<std::string::const_iterator> results;
                                string airp;
                                int weight = NoExists;
                                string mail;
                                if (
                                        boost::regex_match(line, results, uws_line_airp) or
                                        boost::regex_match(line, results, uws_line_bulk)
                                        ) {
                                    airp = GetAirp(results[1]);
                                    weight = ToInt(results[2]);
                                    mail = results[3];
                                } else if (boost::regex_match(line, results, uws_line)) {
                                    weight = ToInt(results[1]);
                                    mail = results[2];
                                }
                                if(weight != NoExists)
                                    con.data[airp][mail == "M"] += weight;
                            }
                            break;
                        default:
                            break;
                    }
                    line_p=tlg.NextLine(line_p);
                } while (line_p and *line_p != 0);
            }
            catch (const ETlgError& E)
            {
                throwTlgError(E.what(), body, line_p);
            };
        }

        void SaveUWSContent(int tlg_id, TAHMHeadingInfo& info, TUWSContent& con) {
            int point_id_tlg=SaveFlt(tlg_id,info.flt,info.bind_type,TSearchFltInfoPtr());

            TQuery Qry(&OraSession);
            Qry.SQLText =
                "SELECT point_id_spp FROM tlg_binding WHERE point_id_tlg=:point_id";
            Qry.CreateVariable("point_id", otInteger, point_id_tlg);
            Qry.Execute();
            if ( Qry.Eof ) {
                throw EXCEPTIONS::Exception( "Flight not found, point_id_tlg=%d", point_id_tlg );
            }
            int point_id_spp = Qry.FieldAsInteger( "point_id_spp" );

            TTripRoute route;
            route.GetRouteAfter(NoExists, point_id_spp, trtNotCurrent, trtNotCancelled);

            // Надо избавиться от пустых АП
            AirpMap result_data;
            for(const auto &airp: con.data) {
                string tmp_airp = airp.first;
                if(tmp_airp.empty())
                    tmp_airp = route[route.size() - 1].airp;
                else {
                    // Проверим, что АП из данных ест в маршруте
                    auto i = route.begin();
                    for(; i != route.end(); i++)
                        if(airp.first == i->airp)
                            break;
                    if(i == route.end())
                        throw ETlgError("Airp %s not found in flight route", airp.first.c_str());
                }
                for(const auto &pr_mail: airp.second)
                    result_data[tmp_airp][pr_mail.first] += pr_mail.second;
            }

            for(auto i = route.begin(); i != route.end(); i++) {
                int cargo = 0;
                int mail = 0;
                auto j = result_data.find(i->airp);
                if(j != result_data.end()) {
                    for(const auto &pr_mail: j->second) {
                        if(pr_mail.first)
                            mail = pr_mail.second;
                        else
                            cargo = pr_mail.second;
                    }
                }
                CargoMailWeight("UWS: ", point_id_spp, i->point_id, cargo, mail);
            }
        }
    }
}
