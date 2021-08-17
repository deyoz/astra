#include "stat/stat_common.h"
#include "stat/stat_utils.h"
#include "date_time.h"
#include "term_version.h"
#include "docs/docs_common.h"
#include "md5_sum.h"
#include "qrys.h"

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
    "statRem",
    "statLimitedCapab",
    "statUnaccBag",
    "statAnnulBT",
    "statPFSShort",
    "statPFSFull",
    "statTrferPax",
    "statHAShort",
    "statHAFull",
    "statBIFull",
    "statBIShort",
    "statBIDetail",
    "statVOFull",
    "statVOShort",
    "statADFull",
    "statReprintShort",
    "statReprintFull",
    "statServicesFull",
    "statServicesShort",
    "statServicesDetail",
    "statSalonFull",
    "statZamarFull",
    "statAHM"
};

void TStatParams::fromFileParams(map<string, string> &file_params)
{
    airp_terminal = ToInt(file_params[PARAM_AIRP_TERMINAL]);
    bi_hall = ToInt(file_params[PARAM_BI_HALL]);
    trfer_airp = file_params[PARAM_TRFER_AIRP];
    trfer_airline = file_params[PARAM_TRFER_AIRLINE];
    seg_category = TSegCategory().decode(file_params[PARAM_SEG_CATEGORY]);
    salon_op_type = file_params[PARAM_SALON_OP_TYPE];
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
            airps.add_elem(i->second);
    }
    ak = file_params[PARAM_AK];
    ap = file_params[PARAM_AP];
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
    LT = ToInt(file_params[PARAM_LT]) != 0;
}

void TStatParams::toFileParams(map<string, string> &file_params) const
{
    file_params[PARAM_AIRP_TERMINAL] = IntToString(airp_terminal);
    file_params[PARAM_BI_HALL] = IntToString(bi_hall);
    file_params[PARAM_TRFER_AIRP] = trfer_airp;
    file_params[PARAM_TRFER_AIRLINE] = trfer_airline;
    file_params[PARAM_SEG_CATEGORY] = TSegCategory().encode(seg_category);
    file_params[PARAM_SALON_OP_TYPE] = salon_op_type;
    file_params[PARAM_SEANCE_TYPE] = IntToString(seance);
    file_params[PARAM_DESK_CITY] = TReqInfo::Instance()->desk.city;
    file_params[PARAM_DESK_LANG] = TReqInfo::Instance()->desk.lang;
    file_params[PARAM_NAME] = name;
    file_params[PARAM_TYPE] = type;
    file_params[PARAM_STAT_TYPE] = EncodeStatType(statType);
    file_params[PARAM_STAT_TYPE] = EncodeStatType(statType);
    file_params[PARAM_AK] = ak;
    file_params[PARAM_AP] = ap;
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
    file_params[PARAM_USER_ID] = IntToString(TReqInfo::Instance()->user.user_id);
    file_params[PARAM_USER_LOGIN] = user_login;
    file_params[PARAM_TYPEB_TYPE] = typeb_type;
    file_params[PARAM_SENDER_ADDR] = sender_addr;
    file_params[PARAM_RECEIVER_DESCR] = receiver_descr;
    file_params[PARAM_REG_TYPE] = reg_type;
    file_params[PARAM_SKIP_ROWS] = IntToString(skip_rows);

    file_params[PARAM_ORDER_SOURCE] = EncodeOrderSource(osSTAT);
    file_params[PARAM_PR_PACTS] = IntToString(pr_pacts);
    file_params[PARAM_LT] = IntToString(LT);
}

void TStatParams::get(xmlNodePtr reqNode)
{
    name = NodeAsString("stat_mode", reqNode);
    type = NodeAsString("stat_type", reqNode, "����");

    if(type == "�࠭���") {
        if(name == "����") statType = statTrferFull;
        else if(name == "���஡���") statType=statTrferPax;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "����") {
        if(name == "���஡���") statType=statFull;
        else if(name == "����") statType=statShort;
        else if(name == "��⠫���஢�����") statType=statDetail;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "����ॣ������") {
        if(name == "���஡���")
            statType=statSelfCkinFull;
        else if(name == "����")
            statType=statSelfCkinShort;
        else if(name == "��⠫���஢�����")
            statType=statSelfCkinDetail;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "�� ����⠬") {
        if(name == "���஡���")
            statType=statAgentFull;
        else if(name == "����")
            statType=statAgentShort;
        else if(name == "�⮣�")
            statType=statAgentTotal;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "���. ⥫��ࠬ��") {
        if(name == "���஡���")
            statType=statTlgOutFull;
        else if(name == "����")
            statType=statTlgOutShort;
        else if(name == "��⠫���஢�����")
            statType=statTlgOutDetail;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "�������") {
        if(name == "����")
            statType=statPactShort;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "������� RFISC") {
        if(name == "���஡���")
            statType=statRFISC;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "����ન") {
        if(name == "���஡���")
            statType=statRem;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "���. ������.") {
        if(name == "���஡���")
            statType=statLimitedCapab;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "��ᮯ�. �����") {
        if(name == "���஡���")
            statType=statUnaccBag;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "����. ��ન") {
        if(name == "���஡���")
            statType=statAnnulBT;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "PFS") {
        if(name == "���஡���")
            statType = statPFSFull;
        else if(name == "����")
            statType = statPFSShort;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "���ᥫ����") {
        if(name == "���஡���")
            statType = statHAFull;
        else if(name == "����")
            statType = statHAShort;
        else
            throw Exception("Unknown stat mode " + name);
    } else if(type == "������ �ਣ��襭��") {
        if(name == "���஡���") statType=statBIFull;
        else if(name == "����") statType=statBIShort;
        else if(name == "��⠫���஢�����") statType=statBIDetail;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "������") {
        if(name == "���஡���") statType=statVOFull;
        else if(name == "����") statType=statVOShort;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "����. �뫥�") {
        if(name == "���஡���") statType=statADFull;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "���ਭ�") {
        if(name == "���஡���") statType=statReprintFull;
        else if(name == "����") statType=statReprintShort;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "��㣨") {
        if(name == "���஡���") statType=statServicesFull;
        else if(name == "����") statType=statServicesShort;
        else if(name == "��⠫���஢�����") statType=statServicesDetail;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "��������� ᠫ���") {
        if(name == "���஡���") statType=statSalonFull;
        else throw Exception("Unknown stat mode " + name);
    } else if(type == "SBDO (Zamar)") {
        if(name == "���஡���") statType=statZamarFull;
        else throw Exception("Unknown stat mode " + name);
    } else
        throw Exception("Unknown stat type " + type);

    FirstDate = NodeAsDateTime("FirstDate", reqNode);
    LastDate = NodeAsDateTime("LastDate", reqNode);
    TReqInfo &info = *(TReqInfo::Instance());

    xmlNodePtr curNode = reqNode->children;

    ak = NodeAsStringFast("ak", curNode, "");
    ap = NodeAsStringFast("ap", curNode, "");
    if (!NodeIsNULLFast("flt_no", curNode, true))
      flt_no = NodeAsIntegerFast("flt_no", curNode, NoExists);
    else
      flt_no = NoExists;
    desk = NodeAsStringFast("desk", curNode, NodeAsStringFast("kiosk", curNode, ""));
    user_login = NodeAsStringFast("user_login", curNode, "");
    typeb_type = NodeAsStringFast("typeb_type", curNode, "");
    sender_addr = NodeAsStringFast("sender_addr", curNode, "");
    receiver_descr = NodeAsStringFast("receiver_descr", curNode, "");
    reg_type = NodeAsStringFast("reg_type", curNode, "");
    order = NodeAsStringFast("Order", curNode, 0) != 0;
    seg_category = TSegCategory().decode(NodeAsStringFast("SegCategory", curNode, ""));
    salon_op_type = NodeAsStringFast("SalonOpType", curNode, "");
    TElemFmt fmt;
    trfer_airp = ElemToElemId(etAirp, NodeAsStringFast("trfer_airp", curNode, ""), fmt);
    trfer_airline = ElemToElemId(etAirline, NodeAsStringFast("trfer_airline", curNode, ""), fmt);
    airp_terminal = NodeAsIntegerFast("terminal", curNode, NoExists);
    bi_hall = NodeAsIntegerFast("bi_hall", curNode, NoExists);

    ProgTrace(TRACE5, "ak: %s", ak.c_str());
    ProgTrace(TRACE5, "ap: %s", ap.c_str());

    airlines=info.user.access.airlines();
    if (
            !ak.empty() and
            statType != statReprintShort and
            statType != statReprintFull
       )
      airlines.merge(TAccessElems<string>(ak, true));
    airps=info.user.access.airps();
    if (!ap.empty())
      airps.merge(TAccessElems<string>(ap, true));

    if (airlines.totally_not_permitted() ||
        airps.totally_not_permitted())
      throw AstraLocale::UserException("MSG.NO_ACCESS");

    airp_column_first = (info.user.user_type == utAirport);

    //ᥠ��� (��������)
    string seance_str = NodeAsStringFast("seance", curNode, "");
    seance = seanceAll;
    if (seance_str=="��") seance=seanceAirline;
    if (seance_str=="��") seance=seanceAirport;
    LogTrace(TRACE6) << __func__ << ": seance_str=" << seance_str;
    LogTrace(TRACE6) << __func__ << ": seance=" << seance;

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

    LT = NodeAsIntegerFast("LTCkBox", curNode, 0) != 0;
};

void TStatParams::AccessClause(
        string &SQLText,
        const string &tab,
        const string &airline_col,
        const string &airp_col
        ) const
{
    if (!airps.elems().empty()) {
        if (airps.elems_permit())
            SQLText += " " + (tab.empty() ? tab : tab + ".") + airp_col + " IN " + GetSQLEnum(airps.elems()) + "and ";
        else
            SQLText += " " + (tab.empty() ? tab : tab + ".") + airp_col + " NOT IN " + GetSQLEnum(airps.elems()) + "and ";
    };
    if (!airlines.elems().empty()) {
        if (airlines.elems_permit())
            SQLText += " " + (tab.empty() ? tab : tab + ".") + airline_col + " IN " + GetSQLEnum(airlines.elems()) + "and ";
        else
            SQLText += " " + (tab.empty() ? tab : tab + ".") + airline_col + " NOT IN " + GetSQLEnum(airlines.elems()) + "and ";
    };
}

bool TStatParams::accessGranted(const TTripInfo& fltInfo) const
{
  if (!airps.elems().empty()) {
      if (airps.elems_permit()) {
          if (airps.elems().find(fltInfo.airp) == airps.elems().end()) {
              return false;
          }
      } else {
          if (airps.elems().find(fltInfo.airp) != airps.elems().end()) {
              return false;
          }
      }
  }
  if (!airlines.elems().empty()) {
      if (airlines.elems_permit()) {
         if (airlines.elems().find(fltInfo.airline) == airlines.elems().end()) {
             return false;
         }
      } else {
         if (airlines.elems().find(fltInfo.airline) != airlines.elems().end()) {
             return false;
         }
      }
  }
  return true;
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
    DB::TCachedQuery updDataQry(
          PgOra::getRWSession("STAT_ORDERS_DATA"),
          "UPDATE stat_orders_data SET "
          "   file_size = :file_size, "
          "   file_size_zip = :file_size_zip, "
          "   md5_sum = :md5_sum "
          "WHERE "
          "   file_id = :file_id and "
          "   month = :month ",
          QParams()
          << QParam("file_id", otInteger, file_id)
          << QParam("month", otDate, month)
          << QParam("file_size", otFloat, data_size)
          << QParam("file_size_zip", otFloat, data_size_zip)
          << QParam("md5_sum", otString, TMD5Sum::Instance()->str()),
          STDLOG);
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
    if(rowcount == 0) { // ���� �����, ���뢠�� 䠩�
        TMD5Sum::Instance()->init();
        out.push(boost::iostreams::zlib_compressor(), ORDERS_BLOCK_SIZE());
        out.push(TMD5Filter(), ORDERS_BLOCK_SIZE());
        out.push(boost::iostreams::file_sink(get_part_file_name(file_id, month)), ORDERS_BLOCK_SIZE());

        QParams qryParams;
        qryParams << QParam("file_id", otInteger, file_id)
                  << QParam("month", otDate, month)
                  << QParam("file_name", otString, file_name);
        DB::TCachedQuery updDataQry(
              PgOra::getRWSession("STAT_ORDERS_DATA"),
              "UPDATE stat_orders_data SET "
              "    file_name = :file_name "
              "WHERE "
              "    file_id = :file_id AND "
              "    month = :month ",
              qryParams, STDLOG);
        updDataQry.get().Execute();

        if (updDataQry.get().RowsProcessed() == 0) {
          DB::TCachedQuery insDataQry(
                PgOra::getRWSession("STAT_ORDERS_DATA"),
                "INSERT INTO stat_orders_data(file_id, month, file_name, download_times) "
                "VALUES (:file_id, :month, :file_name, 0) ",
                qryParams, STDLOG);
          insDataQry.get().Execute();
        }
        ASTRA::commit();
        //ASTRA::commit(); // �⮡� ᡮ�騪 ���� �� ����� � �� 㤠��.

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

const TFltInfoCacheItem &TFltInfoCache::get(int point_id, TDateTime part_key)
{
    TFltInfoCache::iterator i = this->find(point_id);
    if(i == this->end()) {
        TTripInfo info;
        if(part_key != NoExists)
            info.getByPointId(DateTimeToBoost(part_key), point_id);
        else
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

string TAirpArvInfo::get(DB::TQuery &Qry)
{
    int grp_id = Qry.FieldAsInteger("grp_id");
    map<int, string>::iterator im = items.find(grp_id);
    if(im == items.end()) {
        TCkinRoute tckin_route;
        tckin_route.getRouteAfter(GrpId_t(grp_id),
                                  TCkinRoute::WithCurrent,
                                  TCkinRoute::IgnoreDependence,
                                  TCkinRoute::WithoutTransit);
        string airp_arv;
        if(tckin_route.empty())
            airp_arv = Qry.FieldAsString("airp_arv");
        else
            airp_arv = tckin_route.back().airp_arv;
        pair<map<int, string>::iterator, bool> res = items.insert(make_pair(grp_id, airp_arv));
        im = res.first;
    }
    return im->second;
}

string TAirpArvInfo::get(TQuery &Qry)
{
    int grp_id = Qry.FieldAsInteger("grp_id");
    map<int, string>::iterator im = items.find(grp_id);
    if(im == items.end()) {
        TCkinRoute tckin_route;
        tckin_route.getRouteAfter(GrpId_t(grp_id),
                                  TCkinRoute::WithCurrent,
                                  TCkinRoute::IgnoreDependence,
                                  TCkinRoute::WithoutTransit);
        string airp_arv;
        if(tckin_route.empty())
            airp_arv = Qry.FieldAsString("airp_arv");
        else
            airp_arv = tckin_route.back().airp_arv;
        pair<map<int, string>::iterator, bool> res = items.insert(make_pair(grp_id, airp_arv));
        im = res.first;
    }
    return im->second;
}

int MAX_STAT_ROWS()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("MAX_STAT_ROWS",NoExists,NoExists,2000);
  return VAR;
};

int MAX_STAT_SECONDS()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("MAX_STAT_SECONDS",NoExists,NoExists,30);
  return VAR;
};

void TStatOverflow::trace(TRACE_SIGNATURE) const
{
    LogTrace(TRACE_PARAMS) << "TStatOverflow::trace: " << tm.Print();
}

void TStatOverflow::check(size_t RowCount) const
{
    if(apply_type == apply) {
        if(RowCount > (size_t)MAX_STAT_ROWS())
            throw StatOverflowException("MSG.TOO_MANY_ROWS_SELECTED.UPDATE_TERM");
        if(tm.Print() > MAX_STAT_SECONDS() * 1000)
            throw StatOverflowException("MSG.TIMEOUT_EXPIRED.UPDATE_TERM");
    }
}

TStatParams::TStatParams(TStatOverflow::Enum apply_type): overflow(apply_type)
{
}

std::optional<std::string> UsersReader::getDescr(int user_id) const
{
    return algo::find_opt<std::optional>(idDescriptions, user_id);
}

std::optional<int> UsersReader::getUserId(const std::string& login) const
{
    if(!login.empty()) {
        return algo::find_opt<std::optional>(loginIds,login);
    } else{
        LogTrace5 << __func__ << " Empty login!";
        //throw EXCEPTIONS::Exception("NOT FOUND USER_ID. INVALID LOGIN!");
        return std::nullopt;
    }
}

bool UsersReader::containsUser(int user_id) const
{
    return algo::contains(idDescriptions, user_id);
}

void UsersReader::readAllUsers()
{
    int user_id;
    std::string descr;
    std::string login;
    auto cur = make_db_curs("select USER_ID, DESCR, LOGIN from USERS2", PgOra::getROSession("USERS2"));
    cur.def(user_id).def(descr).defNull(login, "").exec();
    while(!cur.fen()) {
        idDescriptions.insert({user_id, descr});
        if(!login.empty()) {
            loginIds.insert({login, user_id});
            login = "";
        }
    }
}

void UsersReader::updateUsers()
{
    int max_user_id = 0;
    if(!idDescriptions.empty()) {
        max_user_id = idDescriptions.rbegin()->first;
        LogTrace5 << " max_user: " << max_user_id;
    }
    int user_id;
    std::string descr;
    std::string login;
    auto cur = make_db_curs("select USER_ID, DESCR, LOGIN from USERS2 where user_id > :max_user_id",
                            PgOra::getROSession("USERS2"));
    cur.def(user_id).def(descr).defNull(login, "").bind(":max_user_id", max_user_id).exec();
    while(!cur.fen()) {
        idDescriptions.insert({user_id, descr});
        if(!login.empty()) {
            loginIds.insert({login, user_id});
            login = "";
        }
    }
}
