#ifndef _STAT_COMMON_H_
#define _STAT_COMMON_H_

#include <string>
#include "astra_utils.h"

namespace STAT {
    static const std::string PARAM_SEANCE_TYPE           = "seance_type";
    static const std::string PARAM_DESK_CITY             = "desk_city";
    static const std::string PARAM_DESK_LANG             = "desk_lang";
    static const std::string PARAM_NAME                  = "name";
    static const std::string PARAM_TYPE                  = "type";
    static const std::string PARAM_STAT_TYPE             = "stat_type";
    static const std::string PARAM_AIRLINES_PREFIX       = "airlines.";
    static const std::string PARAM_AIRLINES_PERMIT       = "airlines.permit";
    static const std::string PARAM_AIRPS_PREFIX          = "airps.";
    static const std::string PARAM_AIRPS_PERMIT          = "airps.permit";
    static const std::string PARAM_AIRP_COLUMN_FIRST     = "airp_column_first";
    static const std::string PARAM_FIRSTDATE             = "FirstDate";
    static const std::string PARAM_LASTDATE              = "LastDate";
    static const std::string PARAM_FLT_NO                = "flt_no";
    static const std::string PARAM_DESK                  = "desk";
    static const std::string PARAM_USER_ID               = "user_id";
    static const std::string PARAM_USER_LOGIN            = "user_login";
    static const std::string PARAM_TYPEB_TYPE            = "typeb_type";
    static const std::string PARAM_SENDER_ADDR           = "sender_addr";
    static const std::string PARAM_RECEIVER_DESCR        = "receiver_descr";
    static const std::string PARAM_REG_TYPE              = "reg_type";
    static const std::string PARAM_SKIP_ROWS             = "skip_rows";
    static const std::string PARAM_ORDER_SOURCE          = "order_source";
    static const std::string PARAM_PR_PACTS              = "pr_pacts";
    static const std::string PARAM_TRFER_AIRP            = "trfer_airp";
    static const std::string PARAM_TRFER_AIRLINE         = "trfer_airline";
    static const std::string PARAM_SEG_CATEGORY          = "seg_category";
    static const std::string PARAM_AIRP_TERMINAL         = "airp_terminal";
    static const std::string PARAM_BI_HALL               = "bi_hall";
}

enum TSeanceType { seanceAirline, seanceAirport, seanceAll };

// Новые типы добавлять в конец списка!
// Чтобы stat.fr3 с ума не сходил
enum TStatType {
    statTrferFull,
    statFull,
    statShort,
    statDetail,
    statSelfCkinFull,
    statSelfCkinShort,
    statSelfCkinDetail,
    statAgentFull,
    statAgentShort,
    statAgentTotal,
    statTlgOutFull,
    statTlgOutShort,
    statTlgOutDetail,
    statPactShort,
    statRFISC,
    statService,
    statLimitedCapab,
    statUnaccBag,
    statAnnulBT,
    statPFSShort,
    statPFSFull,
    statTrferPax,
    statHAShort,
    statHAFull,
    statBIFull,
    statBIShort,
    statBIDetail,
    statVOFull,
    statVOShort,
    statNum
};

extern const char *TStatTypeS[statNum];

class TSegCategories {
    public:
        enum Enum
        {
            IntInt, // Internal - Internal
            ForFor, // Foreign - Foreign
            ForInt, // Foreign -Internal
            IntFor,  // Internal - Foreign
            Unknown
        };

        static const std::list< std::pair<Enum, std::string> >& pairsCodes()
        {
            static std::list< std::pair<Enum, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair(IntInt,  "ВВЛ-ВВЛ"));
                l.push_back(std::make_pair(ForFor,  "МВЛ-МВЛ"));
                l.push_back(std::make_pair(ForInt,  "МВЛ-ВВЛ"));
                l.push_back(std::make_pair(IntFor,  "ВВЛ-МВЛ"));
                l.push_back(std::make_pair(Unknown, ""));
            }
            return l;
        }
};

class TSegCategory : public ASTRA::PairList<TSegCategories::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TSegCategory"; }
  public:
    TSegCategory() : ASTRA::PairList<TSegCategories::Enum, std::string>(TSegCategories::pairsCodes(),
                                                                            boost::none,
                                                                            boost::none) {}
};

struct TStatParams {
    std::string desk_city; // Используется в TReqInfo::Initialize(city) в фоновой статистике
    std::string desk_lang;
    std::string name, type;
    TStatType statType;
    TAccessElems<std::string> airlines, airps;
    bool airp_column_first;
    TSeanceType seance;
    TDateTime FirstDate, LastDate;
    int flt_no;
    std::string desk;
    int user_id;
    std::string user_login;
    std::string typeb_type;
    std::string sender_addr;
    std::string receiver_descr;
    std::string reg_type;
    bool order;
    bool skip_rows;
    bool pr_pacts;
    TSegCategories::Enum seg_category;
    std::string trfer_airp;
    std::string trfer_airline;
    int airp_terminal;
    int bi_hall;
    void get(xmlNodePtr resNode);
    void toFileParams(std::map<std::string, std::string> &file_params) const;
    void fromFileParams(std::map<std::string, std::string> &file_params);
    void AccessClause(std::string &SQLText) const;
};

struct TPrintAirline {
    private:
        std::string val;
        bool multi_airlines;
    public:
        TPrintAirline(): multi_airlines(false) {};
        void check(std::string val);
        std::string get() const;
};

TStatType DecodeStatType( const std::string stat_type );
std::string EncodeStatType(const TStatType stat_type);

enum TOrderSource {
    osSTAT,
    osUnknown,
    osNum
};

extern const char *TOrderSourceS[osNum];

TOrderSource DecodeOrderSource( const std::string &os );
const std::string EncodeOrderSource(TOrderSource s);

int MAX_STAT_ROWS();

namespace AstraLocale {
    class MaxStatRowsException: public UserException
    {
        public: MaxStatRowsException(const std::string &vlexema, const LParams &aparams): UserException(vlexema, aparams) {}
    };
}

struct TOrderStatItem {
    static const char delim = ';';
    virtual void add_header(std::ostringstream &buf) const = 0;
    virtual void add_data(std::ostringstream &buf) const = 0;
    virtual ~TOrderStatItem() {}
};

#include <boost/iostreams/filtering_stream.hpp>

struct TOrderStatWriter {
    const std::string enc;
    int file_id;
    TDateTime month;
    std::string file_name;
    double data_size = 0;
    double data_size_zip = 0;
    boost::iostreams::filtering_ostream out;
    size_t rowcount;

    void push_back(const TOrderStatItem &row)
    {
        insert(row);
    }

    void insert(const TOrderStatItem &row);
    void finish();
    size_t size() { return 0; }

    TOrderStatWriter(int afile_id, TDateTime amonth, const std::string &afile_name):
        enc("CP1251"),
        file_id(afile_id),
        month(amonth),
        file_name(afile_name),
        rowcount(0)
    {}
};

std::string get_part_file_name(int file_id, TDateTime month);

struct TFltInfoCacheItem {
    std::string airp;
    std::string airline;

    std::string view_airp;
    std::string view_airline;
    std::string view_flt_no;
};

struct TFltInfoCache:public std::map<int, TFltInfoCacheItem> {
    const TFltInfoCacheItem &get(int point_id, TDateTime part_key);
};

#endif
