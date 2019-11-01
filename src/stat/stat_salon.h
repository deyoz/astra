#ifndef _STAT_SALON_H_
#define _STAT_SALON_H_

#include "stat_common.h"
#include "astra_locale_adv.h"

class TSalonOpType {
    public:
        static const std::list< std::pair<std::string, std::string> >& pairs()
        {
            static std::list< std::pair<std::string, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair("LAYERSDISABLE",     "Недоступные места"));
                l.push_back(std::make_pair("LAYERSBLOCK_CENT",  "Блокировка центровки"));
                l.push_back(std::make_pair("LAYERSPROTECT",     "Резервирование"));
                l.push_back(std::make_pair("LAYERSSMOKE",       "Место для курящих"));
                l.push_back(std::make_pair("LAYERSUNCOMFORT",   "Неудобные места"));
                l.push_back(std::make_pair("WEB_TARIFF",        "Web-тариф"));
                l.push_back(std::make_pair("REMS",              "Ремарки"));
                l.push_back(std::make_pair("RFISC",             "Разметка RFISC"));
                l.push_back(std::make_pair("SEAT",              "Кресло"));
            }
            return l;
        }
};

class TSalonOpTypes: public ASTRA::PairList<std::string, std::string>
{
    private:
        virtual std::string className() const { return "TSalonOpTypes"; }
    public:
        TSalonOpTypes() : ASTRA::PairList<std::string, std::string>(TSalonOpType::pairs(),
                boost::none,
                boost::none) {}
};

const TSalonOpTypes &SalonOpTypes();

struct TSalonStatRow:public TOrderStatItem {
    int point_id;
    TDateTime scd_out;
    TDateTime time;
    std::string login;
    std::string op_type;
    std::string msg;
    std::string airline;
    int flt_no;
    std::string suffix;

    void clear()
    {
        point_id = ASTRA::NoExists;
        scd_out = ASTRA::NoExists;
        time = ASTRA::NoExists;
        login.clear();
        op_type.clear();
        msg.clear();
        airline.clear();
        flt_no = ASTRA::NoExists;
        suffix.clear();
    }

    TSalonStatRow() { clear(); }

    bool operator < (const TSalonStatRow &val) const
    {
        if(point_id != val.point_id)
            return point_id < val.point_id;
        if(time != val.time)
            return time < val.time;
        return login < val.login;
    }

    void add_header(std::ostringstream &buf) const;
    void add_data(std::ostringstream &buf) const;
};

typedef std::multiset<TSalonStatRow> TSalonStat;

template <class T>
void RunSalonStat(
        const TStatParams &params,
        T &SalonStat,
        TPrintAirline &prn_airline
        );
void createXMLSalonStat(const TStatParams &params, const TSalonStat &SalonStat, const TPrintAirline &prn_airline, xmlNodePtr resNode);

void to_stat_salon(int point_id, const PrmEnum &msg, const std::string &op_type);

#endif
