#ifndef ARX_DAILY_PG_H
#define ARX_DAILY_PG_H

#include "date_time.h"
#include "oralib.h"
#include "astra_dates.h"
#include "astra_types.h"
#include "astra_consts.h"
#include "hooked_session.h"
#include <optional>

#define get_arx_ora_curs(x) get_arx_ora_rw_sess(STDLOG)->createCursor(STDLOG, (x))
#define get_arx_pg_ro_curs(x) get_arx_pg_ro_sess(STDLOG)->createCursor(STDLOG, (x))
#define get_arx_pg_rw_curs(x) get_arx_pg_rw_sess(STDLOG)->createCursor(STDLOG, (x))

#define get_arx_ro_curs(x) PG_ARX::ARX_READ_PG() ? get_arx_pg_ro_curs(x) : get_arx_ora_curs(x)
#define get_arx_rw_curs(x) PG_ARX::ARX_PG_ENABLE() ? get_arx_pg_rw_curs(x) : get_arx_ora_curs(x)

#define get_arx_ro_session() (PG_ARX::ARX_READ_PG() ? get_arx_pg_ro_sess(STDLOG) : get_arx_ora_rw_sess(STDLOG))
#define get_arx_rw_session() (PG_ARX::ARX_PG_ENABLE() ? get_arx_pg_rw_sess(STDLOG) : get_arx_ora_rw_sess(STDLOG))

namespace  PG_ARX {
int ARX_DAYS();
int ARX_DURATION();
int ARX_SLEEP();
int ARX_MAX_ROWS();
bool ARX_PG_ENABLE();
bool ARX_READ_PG();

struct TBagInfo
{
    int grp_id = 0;
    std::optional<int> pax_id;
    int bagAmount = 0; //NUMBER(5)
    int bagWeight = 0; //NUMBER(6),
    int rkAmount = 0; //NUMBER(5),
    int rkWeight = 0; //NUMBER(6)
};

TBagInfo get_bagInfo2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                       std::optional<int> bag_pool_num);
std::optional<int> get_bagAmount2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                    std::optional<int> bag_pool_num);
std::optional<int> get_bagWeight2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                                  std::optional<int> bag_pool_num);
std::optional<int> get_rkAmount2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                    std::optional<int> bag_pool_num);
std::optional<int> get_rkWeight2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                                  std::optional<int> bag_pool_num);
std::optional<int> get_excess_wt(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                                 std::optional<int> excess_wt, std::optional<int> excess_nvl,
                                 int bag_refuse);
std::optional<int> get_bag_pool_pax_id(Dates::DateTime_t part_key, int grp_id,
                                       std::optional<int> bag_pool_num, int include_refused = 1);
int bag_pool_refused(Dates::DateTime_t part_key, int grp_id, int bag_pool_num,
                     std::optional<std::string> vclass, int bag_refuse);
std::optional<std::string> get_birks2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                       int bag_pool_num, const std::string& lang);
std::optional<std::string> get_birks2(Dates::DateTime_t part_key, int grp_id, std::optional<int> pax_id,
                       int bag_pool_num, int pr_lat = 0);
std::optional<int> get_main_pax_id2(Dates::DateTime_t part_key, int grp_id, int include_refused = 1);




bool arx_daily(const Dates::DateTime_t &utcdate);
#ifdef XP_TESTING
bool test_arx_daily(const Dates::DateTime_t &utcdate, int step);
#endif

class TArxMove
{
  protected:
    int proc_count;
    Dates::DateTime_t utcdate;
  public:
    TArxMove(const Dates::DateTime_t& utc_date);
    virtual ~TArxMove();
    virtual void BeforeProc() { proc_count=0; }
    virtual bool Next(size_t max_rows, int duration) = 0; //duration - длительность итерации в сек.
    virtual std::string TraceCaption() = 0;
    int Processed() { return proc_count; }
};

class TArxMoveFlt : public TArxMove
{
  private:
    int step, move_ids_count;
    std::map<MoveId_t, Dates::DateTime_t> move_ids;
  protected:
    bool GetPartKey(const MoveId_t& move_id, Dates::DateTime_t &part_key, double &date_range);
    void LockAndCollectStat(const MoveId_t& move_id);
    void readMoveIds(size_t max_rows);
  public:
    TArxMoveFlt(const Dates::DateTime_t& utc_date);
    virtual ~TArxMoveFlt();
    virtual bool Next(size_t max_rows, int duration);
    virtual std::string TraceCaption();

};

class TArxTypeBIn : public TArxMove
{
  public:
    TArxTypeBIn(const Dates::DateTime_t& utc_date);
    virtual ~TArxTypeBIn() = default;
    virtual bool Next(size_t max_rows, int duration);
    virtual std::string TraceCaption();

};

class TArxTlgTrips : public TArxMove
{
  private:
    int step, point_ids_count;
  public:
    std::vector<PointId_t> getTlgTripPoints(const Dates::DateTime_t &arx_date, size_t max_rows);
    TArxTlgTrips(const Dates::DateTime_t& utc_date);
    virtual bool Next(size_t max_rows, int duration);
    virtual std::string TraceCaption();
};

class TArxMoveNoFlt : public TArxMove
{
protected:
    int step;
public:
    TArxMoveNoFlt(const Dates::DateTime_t& utc_date);
    virtual ~TArxMoveNoFlt();
    virtual bool Next(size_t max_rows, int duration);
    virtual std::string TraceCaption();
};

class TArxNormsRatesEtc : public TArxMove
{
protected:
    int step;
public:
    TArxNormsRatesEtc(const Dates::DateTime_t& utc_date);
    virtual ~TArxNormsRatesEtc() = default;
    virtual bool Next(size_t max_rows, int duration);
    virtual std::string TraceCaption();
};

class TArxTlgsFilesEtc : public TArxMove
{
protected:
    int step;
public:
    TArxTlgsFilesEtc(const Dates::DateTime_t& utc_date);
    virtual ~TArxTlgsFilesEtc() = default;
    virtual bool Next(size_t max_rows, int duration);
    virtual std::string TraceCaption();
};

}
#endif // ARX_DAILY_PG_H
