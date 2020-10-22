#ifndef ARX_DAILY_PG_H
#define ARX_DAILY_PG_H

#include "date_time.h"
#include "oralib.h"
#include "astra_dates.h"
#include "astra_types.h"

namespace  PG_ARX {

bool ARX_PG_ENABLE();
int ARX_DAYS();
int ARX_DURATION();
int ARX_SLEEP();
int ARX_MAX_ROWS();

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
