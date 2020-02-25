#include "stat_seDCSAddReport.h"
#include <iostream>
#include "date_time.h"
#include "qrys.h"
#include "astra_utils.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace ASTRA;

int nosir_seDCSAddReport(int argc, char **argv)
{
    if(argc != 2) {
        cout << "usage: " << argv[0] << " yyyymmdd" << endl;
        return 1;
    }

    TDateTime FirstDate;
    if(StrToDateTime(argv[1], "yyyymmdd", FirstDate) == EOF) {
        cout << "wrong date: " << argv[1] << endl;
        return 1;
    }

    struct TFltStat {
        int col_point_id;
        int col_airline;
        int col_airp;
        int col_flt_no;
        int col_suffix;
        int col_scd_out;

        TCachedQuery fltQry;

        const char *delim;


        typedef map<bool, int> TBagRow;
        typedef map<bool, TBagRow> TWebRow;
        typedef map<bool, TWebRow> TPersRow;
        typedef map<int, TPersRow> TFltData;

        // data[point_id][pr_adult][pr_web][pr_bag] = count;
        TFltData data;

        void data_dump()
        {
            for(TFltData::iterator flt = data.begin(); flt != data.end(); flt++) {
                for(TPersRow::iterator pers = flt->second.begin(); pers != flt->second.end(); pers++) {
                    for(TWebRow::iterator web = pers->second.begin(); web != pers->second.end(); web++) {
                        for(TBagRow::iterator bag = web->second.begin(); bag != web->second.end(); bag++) {
                            LogTrace(TRACE5)
                                << "data["
                                << flt->first
                                << "]["
                                << pers->first
                                << "]["
                                << web->first
                                << "]["
                                << bag->first
                                << "] = "
                                << bag->second;
                        }
                    }
                }
            }
        }

        TFltStat(TQuery &Qry, const char *adelim): fltQry(
                "select "
                "    pax_grp.client_type, "
                "    pax.pers_type, "
                "    pax.bag_pool_num pr_bag "
                "from "
                "    pax_grp, "
                "    pax "
                "where "
                "    pax_grp.point_dep = :point_id and "
                "    pax.grp_id = pax_grp.grp_id and "
                "    pax_grp.status not in ('E') ",
                QParams() << QParam("point_id", otInteger)
                ),
        delim(adelim)
        {
            col_point_id = Qry.GetFieldIndex("point_id");
            col_airline = Qry.GetFieldIndex("airline");
            col_airp = Qry.GetFieldIndex("airp");
            col_flt_no = Qry.GetFieldIndex("flt_no");
            col_suffix = Qry.GetFieldIndex("suffix");
            col_scd_out = Qry.GetFieldIndex("scd_out");
        }

        void get(TQuery &Qry, ofstream &of) {
            data.clear();
            int point_id = Qry.FieldAsInteger(col_point_id);
            fltQry.get().SetVariable("point_id", point_id);
            fltQry.get().Execute();
            if(not fltQry.get().Eof) {
                int col_client_type = fltQry.get().GetFieldIndex("client_type");
                int col_pers_type = fltQry.get().GetFieldIndex("pers_type");
                int col_pr_bag = fltQry.get().GetFieldIndex("pr_bag");
                for(; not fltQry.get().Eof; fltQry.get().Next()) {
                    bool pr_adult = DecodePerson(fltQry.get().FieldAsString(col_pers_type)) == adult;
                    bool pr_web = DecodeClientType(fltQry.get().FieldAsString(col_client_type)) != ctTerm;
                    bool pr_bag = not fltQry.get().FieldIsNULL(col_pr_bag);
                    data[point_id][pr_adult][pr_web][pr_bag]++;
                }
            }
            if(not data.empty()) {
                data_dump();
                of
                    //Код аэропорта (города)
                    << Qry.FieldAsString(col_airp) << delim
                    //Перевозчик
                    << Qry.FieldAsString(col_airline) << delim
                    //Номер рейса
                    << Qry.FieldAsString(col_flt_no) << delim
                    //Литера
                    << Qry.FieldAsString(col_suffix) << delim
                    //Дата рейса
                    << DateTimeToStr(Qry.FieldAsDateTime(col_scd_out), "dd.mm.yyyy") << delim
                    //Пассажиры ВЗР с регистрацией в а/п
                    <<
                    data[point_id][true][false][false] +
                    data[point_id][true][false][true]
                    << delim
                    //Пассажиры РБ с регистрацией в а/п
                    <<
                    data[point_id][false][false][false] +
                    data[point_id][false][false][true]
                    << delim
                    //Пассажиры ВЗР с регистрацией в а/п без багажа
                    <<
                    data[point_id][true][false][false]
                    << delim
                    //Пассажиры РБ с регистрацией в а/п без багажа
                    <<
                    data[point_id][false][false][false]
                    << delim
                    //Пассажиры ВЗР с саморегистрацией и багажом
                    <<
                    data[point_id][true][true][true]
                    << delim
                    //Пассажиры РБ с саморегистрацией и багажом
                    <<
                    data[point_id][false][true][true]
                    << delim
                    //Пассажиры ВЗР с саморегистрацией без багажа
                    <<
                    data[point_id][true][true][false]
                    << delim
                    //Пассажиры РБ с саморегистрацией без багажа
                    <<
                    data[point_id][false][true][false]
                    << endl;
            }
        }
    };

    TCachedQuery Qry(
            "select "
            "   point_id, "
            "   airline, "
            "   airp, "
            "   flt_no, "
            "   suffix, "
            "   scd_out "
            "from "
            "   points "
            "where "
            "   scd_out>=:first_date AND scd_out<:last_date AND airline='ЮТ' AND "
            "   pr_reg<>0 AND pr_del>=0 ",
            QParams()
            << QParam("first_date", otDate, FirstDate)
            << QParam("last_date", otDate, FirstDate + 1)
            );

    Qry.get().Execute();
    if(not Qry.get().Eof) {
        const char *delim = ",";
        ofstream of(((string)"seDCSAddReport." + argv[1] + ".csv").c_str());
        of
            << "Код аэропорта (города)" << delim
            << "Перевозчик" << delim
            << "Номер рейса" << delim
            << "Литера" << delim
            << "Дата рейса" << delim
            << "Пассажиры ВЗР с регистрацией в а/п" << delim
            << "Пассажиры РБ с регистрацией в а/п" << delim
            << "Пассажиры ВЗР с регистрацией в а/п без багажа" << delim
            << "Пассажиры РБ с регистрацией в а/п без багажа" << delim
            << "Пассажиры ВЗР с саморегистрацией и багажом" << delim
            << "Пассажиры РБ с саморегистрацией и багажом" << delim
            << "Пассажиры ВЗР с саморегистрацией без багажа" << delim
            << "Пассажиры РБ с саморегистрацией без багажа" << endl;
        TFltStat flt_stat(Qry.get(), delim);
        for(; not Qry.get().Eof; Qry.get().Next())
            flt_stat.get(Qry.get(), of);
    }

    return 1;
}

