#include "stat_common.h"
#include "stat_utils.h"
#include "date_time.h"
#include "term_version.h"
#include "docs.h"
#include "md5_sum.h"

#define NICKNAME "DENIS"
#include "serverlib/slogger.h"

using namespace std;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace ASTRA;
using namespace STAT;
using namespace AstraLocale;

const char *TOrderSourceS[] = {
    "STAT",
    "?"
};

const char *TStatTypeS[statNum] = {
    "statTrferFull",
    "statFull",
    "statShort",
    "statDetail",
    "statSelfCkinFull",
    "statSelfCkinShort",
    "statSelfCkinDetail",
    "statAgentFull",
    "statAgentShort",
    "statAgentTotal",
    "statTlgOutFull",
    "statTlgOutShort",
    "statTlgOutDetail",
    "statPactShort",
    "statRFISC",
    "statService",
    "statLimitedCapab",
    "statUnaccBag",
    "statAnnulBT",
    "statPFSShort",
    "statPFSFull",
    "statTrferPax",
    "statHotelAcmdShort",
    "statHotelAcmdFull",
    "statBIFull",
    "statBIShort",
    "statBIDetail",
    "statVOFull",
    "statVOShort"
};

void TStatParams::fromFileParams(map<string, string> &file_params)
{
    airp_terminal = ToInt(file_params[PARAM_AIRP_TERMINAL]);
    bi_hall = ToInt(file_params[PARAM_BI_HALL]);
    trfer_airp = file_params[PARAM_TRFER_AIRP];
    trfer_airline = file_params[PARAM_TRFER_AIRLINE];
    seg_category = TSegCategory().decode(file_params[PARAM_SEG_CATEGORY]);
    seance = (TSeanceType)ToInt(file_params[PARAM_SEANCE_TYPE]);
    desk_city = file_params[PARAM_DESK_CITY];
    desk_lang = file_params[PARAM_DESK_LANG];
    name = file_params[PARAM_NAME];
    type = file_params[PARAM_TYPE];
    statType = DecodeStatType(file_params[PARAM_STAT_TYPE]);
    for(map<string, string>::iterator i = file_params.begin(); i != file_params.end(); i++) {
        if(i->first.substr(0, PARAM_AIRLINES_PREFIX.size()) == PARAM_AIRLINES_PREFIX)
            airlines.add_elem(i->second);
        if(i->first.substr(0, PARAM_AIRPS_PREFIX.size()) == PARAM_AIRPS_PREFIX)
            airlines.add_elem(i->second);
    }
    airlines.set_elems_permit(ToInt(file_params[PARAM_AIRLINES_PERMIT]));
    airps.set_elems_permit(ToInt(file_params[PARAM_AIRPS_PERMIT]));
    airp_column_first = ToInt(file_params[PARAM_AIRP_COLUMN_FIRST]);
    StrToDateTime(file_params[PARAM_FIRSTDATE].c_str(), ServerFormatDateTimeAsString, FirstDate);
    StrToDateTime(file_params[PARAM_LASTDATE].c_str(), ServerFormatDateTimeAsString, LastDate);
    flt_no = ToInt(file_params[PARAM_FLT_NO]);
    desk = file_params[PARAM_DESK];
    user_id = ToInt(file_params[PARAM_USER_ID]);
    user_login = file_params[PARAM_USER_LOGIN];
    typeb_type = file_params[PARAM_TYPEB_TYPE];
    sender_addr = file_params[PARAM_SENDER_ADDR];
    receiver_descr = file_params[PARAM_RECEIVER_DESCR];
    reg_type = file_params[PARAM_REG_TYPE];
    skip_rows = ToInt(file_params[PARAM_SKIP_ROWS]);
    pr_pacts = ToInt(file_params[PARAM_PR_PACTS]) != 0;
}

void TStatParams::toFileParams(map<string, string> &file_params) const
{
    file_params[PARAM_AIRP_TERMINAL] = IntToString(airp_terminal);
    file_params[PARAM_BI_HALL] = IntToString(bi_hall);
    file_params[PARAM_TRFER_AIRP] = trfer_airp;
    file_params[PARAM_TRFER_AIRLINE] = trfer_airline;
    file_params[PARAM_SEG_CATEGORY] = TSegCategory().encode(seg_category);
    file_params[PARAM_SEANCE_TYPE] = IntToString(seance);
    file_params[PARAM_DESK_CITY] = TReqInfo::Instance()->desk.city;
    file_params[PARAM_DESK_LANG] = TReqInfo::Instance()->desk.lang;
    file_params[PARAM_NAME] = name;
    file_params[PARAM_TYPE] = type;
    file_params[PARAM_STAT_TYPE] = EncodeStatType(statType);
    file_params[PARAM_STAT_TYPE] = EncodeStatType(statType);
    int idx = 0;
    for(set<string>::iterator i = airlines.elems().begin(); i != airlines.elems().end(); i++, idx++) {
        file_params[PARAM_AIRLINES_PREFIX + IntToString(idx)] = *i;
    }
    file_params[PARAM_AIRLINES_PERMIT] = IntToString(airlines.elems_permit());

    idx = 0;
    for(set<string>::iterator i = airps.elems().begin(); i != airps.elems().end(); i++, idx++) {
        file_params[PARAM_AIRPS_PREFIX + IntToString(idx)] = *i;
    }
    file_params[PARAM_AIRPS_PERMIT] = IntToString(airps.elems_permit());
    file_params[PARAM_AIRP_COLUMN_FIRST] = IntToString(airp_column_first);
    file_params[PARAM_FIRSTDATE] = DateTimeToStr(FirstDate, ServerFormatDateTimeAsString);
    file_params[PARAM_LASTDATE] = DateTimeToStr(LastDate, ServerFormatDateTimeAsString);
    file_params[PARAM_FLT_NO] = IntToString(flt_no);
    file_params[PARAM_DESK] = desk;
    file_params[PARAM_USER_ID] = IntToString(user_id);
    file_params[PARAM_USER_LOGIN] = user_login;
    file_params[PARAM_TYPEB_TYPE] = typeb_type;
    file_params[PARAM_SENDER_ADDR] = sender_addr;
    file_params[PARAM_RECEIVER_DESCR] = receiver_descr;
    file_params[PARAM_REG_TYPE] = reg_type;
    file_params[PARAM_SKIP_ROWS] = IntToString(skip_rows);

    file_params[PARAM_ORDER_SOURCE] = EncodeOrderSource(osSTAT);
    file_params[PARAM_PR_PACTS] = IntToString(pr_pacts);
}

void TStatParams::get(xmlNodePtr reqNode)
{
    name = NodeAsString("stat_mode", reqNode);
    type = NodeAsString("stat_type", reqNode, "Общая");

    if(type == "Трансфер") {
        if(name == "Общая") statType = statTrferFull;
        else if(name == "Подробная") statType=statTrferPax;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "Общая") {
        if(name == "Подробная") statType=statFull;
        else if(name == "Общая") statType=statShort;
        else if(name == "Детализированная") statType=statDetail;
        else throw Exception("Unknown stat mode " + name);
    } else if(type ==
            ((TReqInfo::Instance()->client_type==ctHTTP ||
              TReqInfo::Instance()->desk.compatible(SELF_CKIN_STAT_VERSION)) ?
             "Саморегистрация" :
             "По киоскам"
            )
            ) {
        if(name == "Подробная")
            statType=statSelfCkinFull;
        else if(name == "Общая")
            statType=statSelfCkinShort;
        else if(name == "Детализированная")
            statType=statSelfCkinDetail;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "По агентам") {
        if(name == "Подробная")
            statType=statAgentFull;
        else if(name == "Общая")
            statType=statAgentShort;
        else if(name == "Итого")
            statType=statAgentTotal;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Отпр. телеграммы") {
        if(name == "Подробная")
            statType=statTlgOutFull;
        else if(name == "Общая")
            statType=statTlgOutShort;
        else if(name == "Детализированная")
            statType=statTlgOutDetail;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Договор") {
        if(name == "Общая")
            statType=statPactShort;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Багажные RFISC") {
        if(name == "Подробная")
            statType=statRFISC;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Услуги") {
        if(name == "Подробная")
            statType=statService;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Огр. возмож.") {
        if(name == "Подробная")
            statType=statLimitedCapab;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Несопр. багаж") {
        if(name == "Подробная")
            statType=statUnaccBag;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Аннул. бирки") {
        if(name == "Подробная")
            statType=statAnnulBT;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "PFS") {
        if(name == "Подробная")
            statType = statPFSFull;
        else if(name == "Общая")
            statType = statPFSShort;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Расселение") {
        if(name == "Подробная")
            statType = statHotelAcmdFull;
        else if(name == "Общая")
            statType = statHotelAcmdShort;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "Бизнес приглашения") {
        if(name == "Подробная") statType=statBIFull;
        else if(name == "Общая") statType=statBIShort;
        else if(name == "Детализированная") statType=statBIDetail;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "Ваучеры") {
        if(name == "Подробная") statType=statVOFull;
        else if(name == "Общая") statType=statVOShort;
        else throw Exception("Unknown stat mode " + name);
    } else
        throw Exception("Unknown stat type " + type);

    FirstDate = NodeAsDateTime("FirstDate", reqNode);
    LastDate = NodeAsDateTime("LastDate", reqNode);
    TReqInfo &info = *(TReqInfo::Instance());

    xmlNodePtr curNode = reqNode->children;

    string ak = NodeAsStringFast("ak", curNode, "");
    string ap = NodeAsStringFast("ap", curNode, "");
    if (!NodeIsNULLFast("flt_no", curNode, true))
      flt_no = NodeAsIntegerFast("flt_no", curNode, NoExists);
    else
      flt_no = NoExists;
    desk = NodeAsStringFast("desk", curNode, NodeAsStringFast("kiosk", curNode, ""));
    if (!NodeIsNULLFast("user", curNode, true))
      user_id = NodeAsIntegerFast("user", curNode, NoExists);
    else
      user_id = NoExists;
    user_login = NodeAsStringFast("user_login", curNode, "");
    typeb_type = NodeAsStringFast("typeb_type", curNode, "");
    sender_addr = NodeAsStringFast("sender_addr", curNode, "");
    receiver_descr = NodeAsStringFast("receiver_descr", curNode, "");
    reg_type = NodeAsStringFast("reg_type", curNode, "");
    order = NodeAsStringFast("Order", curNode, 0) != 0;
    seg_category = TSegCategory().decode(NodeAsStringFast("SegCategory", curNode, ""));
    TElemFmt fmt;
    trfer_airp = ElemToElemId(etAirp, NodeAsStringFast("trfer_airp", curNode, ""), fmt);
    trfer_airline = ElemToElemId(etAirline, NodeAsStringFast("trfer_airline", curNode, ""), fmt);
    airp_terminal = NodeAsIntegerFast("terminal", curNode, NoExists);
    bi_hall = NodeAsIntegerFast("bi_hall", curNode, NoExists);

    ProgTrace(TRACE5, "ak: %s", ak.c_str());
    ProgTrace(TRACE5, "ap: %s", ap.c_str());

    airlines=info.user.access.airlines();
    if (!ak.empty())
      airlines.merge(TAccessElems<string>(ak, true));
    airps=info.user.access.airps();
    if (!ap.empty())
      airps.merge(TAccessElems<string>(ap, true));

    if (airlines.totally_not_permitted() ||
        airps.totally_not_permitted())
      throw AstraLocale::UserException("MSG.NO_ACCESS");

    airp_column_first = (info.user.user_type == utAirport);

    //сеансы (договоры)
    string seance_str = NodeAsStringFast("seance", curNode, "");
    seance = seanceAll;
    if (seance_str=="АК") seance=seanceAirline;
    if (seance_str=="АП") seance=seanceAirport;

    bool all_seances_permit = info.user.access.rights().permitted(615);

    if (info.user.user_type != utSupport && !all_seances_permit)
    {
      if (info.user.user_type == utAirline)
        seance=seanceAirline;
      else
        seance=seanceAirport;
    };

    if (seance==seanceAirline)
    {
        if (!airlines.elems_permit()) throw UserException("MSG.NEED_SET_CODE_AIRLINE");
    };

    if (seance==seanceAirport)
    {
        if (!airps.elems_permit()) throw UserException("MSG.NEED_SET_CODE_AIRP");
    };

    skip_rows =
        info.user.user_type == utAirline and
        statType == statTrferFull and
        ak.empty() and
        ap.empty() and
        flt_no == NoExists and
        seance == seanceAll;
    pr_pacts = false;
};

void TStatParams::AccessClause(string &SQLText) const
{
    if (!airps.elems().empty()) {
        if (airps.elems_permit())
            SQLText += " points.airp IN " + GetSQLEnum(airps.elems()) + "and ";
        else
            SQLText += " points.airp NOT IN " + GetSQLEnum(airps.elems()) + "and ";
    };
    if (!airlines.elems().empty()) {
        if (airlines.elems_permit())
            SQLText += " points.airline IN " + GetSQLEnum(airlines.elems()) + "and ";
        else
            SQLText += " points.airline NOT IN " + GetSQLEnum(airlines.elems()) + "and ";
    };
}

string TPrintAirline::get() const
{
    if(multi_airlines)
        return "";
    else
        return val;
}

void TPrintAirline::check(string val)
{
    if(this->val.empty())
        this->val = val;
    else if(this->val != val)
        multi_airlines = true;
}

TStatType DecodeStatType( const string stat_type )
{
    int i;
    for( i=0; i<(int)statNum; i++ )
        if ( stat_type == TStatTypeS[ i ] )
            break;
    return (TStatType)i;
}

string EncodeStatType(const TStatType stat_type)
{
    return (
            stat_type >= statNum or stat_type < 0
            ? string() : TStatTypeS[ stat_type ]
           );
}

TOrderSource DecodeOrderSource( const string &os )
{
  int i;
  for( i=0; i<(int)osNum; i++ )
    if ( os == TOrderSourceS[ i ] )
      break;
  if ( i == osNum )
    return osUnknown;
  else
    return (TOrderSource)i;
}

const string EncodeOrderSource(TOrderSource s)
{
  return TOrderSourceS[s];
};

int MAX_STAT_ROWS()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("MAX_STAT_ROWS",NoExists,NoExists,2000);
  return VAR;
};

void TOrderStatWriter::finish()
{
    if(rowcount == 0) return;
    // forces all the underlying buffers to be flushed, thus TMD5Sum updates correctly
    //
    // from boost manual:
    //  void reset()
    //  Clears the underlying chain. If the chain is initially complete,
    //  causes each Filter and Device in the chain to be closed using the function close.
    out.reset();
    TMD5Sum::Instance()->Final();
    data_size_zip = TMD5Sum::Instance()->data_size;
    TCachedQuery updDataQry(
            "update stat_orders_data set "
            "   file_size = :file_size, "
            "   file_size_zip = :file_size_zip, "
            "   md5_sum = :md5_sum "
            "where "
            "   file_id = :file_id and "
            "   month = :month ",
            QParams()
            << QParam("file_id", otInteger, file_id)
            << QParam("month", otDate, month)
            << QParam("file_size", otFloat, data_size)
            << QParam("file_size_zip", otFloat, data_size_zip)
            << QParam("md5_sum", otString, TMD5Sum::Instance()->str())
            );
    updDataQry.get().Execute();
}

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/device/file.hpp>

class TMD5Filter : public boost::iostreams::multichar_output_filter {
    public:
        template<typename Sink>
            std::streamsize write(Sink& dest, const char* s, std::streamsize n)
            {
                TMD5Sum::Instance()->update(s, n);
                boost::iostreams::copy(boost::iostreams::basic_array_source<char>(s, n), dest);
                return n;
            }

        /* Other members */
};

string get_part_file_name(int file_id, TDateTime month)
{
    // get part file name
    ostringstream file_name;
    file_name << ORDERS_PATH() << file_id << '.' << DateTimeToStr(month, "yymm");
    return file_name.str();
}

void TOrderStatWriter::insert(const TOrderStatItem &row)
{
    if(rowcount == 0) { // первый инсерт, открываем файл
        TMD5Sum::Instance()->init();
        out.push(boost::iostreams::zlib_compressor(), ORDERS_BLOCK_SIZE());
        out.push(TMD5Filter(), ORDERS_BLOCK_SIZE());
        out.push(boost::iostreams::file_sink(get_part_file_name(file_id, month)), ORDERS_BLOCK_SIZE());

        TCachedQuery insDataQry(
                "begin "
                "   insert into stat_orders_data(file_id, month, file_name, download_times) values ( "
                "   :file_id, :month, :file_name, 0); "
                "exception "
                "   when dup_val_on_index then "
                "       update stat_orders_data set "
                "           file_name = :file_name "
                "       where "
                "           file_id = :file_id and "
                "           month = :month; "
                "end; "
                ,
                QParams() << QParam("file_id", otInteger, file_id)
                << QParam("month", otDate, month)
                << QParam("file_name", otString, file_name)
                );
        insDataQry.get().Execute();
        OraSession.Commit(); // чтобы сборщик мусора все видел и не удалял.

        ostringstream buf;
        row.add_header(buf);
        out << ConvertCodepage(buf.str(), "CP866", enc);
        data_size += buf.str().size();
    }
    ostringstream buf;
    row.add_data(buf);
    rowcount++;
    out << ConvertCodepage(buf.str(), "CP866", enc);
    data_size += buf.str().size();
    out.flush();
}

const TFltInfoCacheItem &TFltInfoCache::get(int point_id)
{
    TFltInfoCache::iterator i = this->find(point_id);
    if(i == this->end()) {
        TTripInfo info;
        info.getByPointId(point_id);
        TFltInfoCacheItem item;
        item.airp = info.airp;
        item.airline = info.airline;
        item.view_airp = ElemIdToCodeNative(etAirp, info.airp);
        item.view_airline = ElemIdToCodeNative(etAirline, info.airline);
        ostringstream flt_no_str;
        flt_no_str << setw(3) << setfill('0') << info.flt_no << ElemIdToCodeNative(etSuffix, info.suffix);
        item.view_flt_no = flt_no_str.str();
        pair<TFltInfoCache::iterator, bool> ret;
        ret = this->insert(make_pair(point_id, item));
        i = ret.first;
    }
    return i->second;
}
