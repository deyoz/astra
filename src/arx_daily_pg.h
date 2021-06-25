#ifndef ARX_DAILY_PG_H
#define ARX_DAILY_PG_H

#include "date_time.h"
#include "oralib.h"
#include "astra_dates.h"
#include "astra_types.h"
#include "astra_consts.h"
#include "hooked_session.h"
#include <optional>

namespace ARX {

bool WRITE_PG();
bool WRITE_ORA();
bool READ_PG();
bool READ_ORA();
bool CLEANUP_PG();
int ARX_DAYS();
int ARX_DURATION();
int ARX_SLEEP();
int ARX_MAX_ROWS();
int ARX_MAX_DATE_RANGE();
}//namespace ARX

/////////////////////////////////////////////////////////////////////////////////////////

bool arx_daily_pg(TDateTime utcdate);

/////////////////////////////////////////////////////////////////////////////////////////

namespace PG_ARX {

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
    virtual ~TArxMove() = default;
    virtual void BeforeProc() { proc_count=0; }
    virtual bool Next(size_t max_rows, int duration) = 0; //duration - ���⥫쭮��� ���樨 � ᥪ.
    virtual std::string TraceCaption() = 0;
    int Processed() { return proc_count; }
};

class TArxMoveFlt : public TArxMove
{
private:
    std::map<MoveId_t, Dates::time_period> move_ids;
protected:
    void LockAndCollectStat(const MoveId_t& move_id);
    void readMoveIds(size_t max_rows);
public:
    TArxMoveFlt(const Dates::DateTime_t& utc_date);
    virtual ~TArxMoveFlt() = default;
    virtual bool Next(size_t max_rows, int duration);
    virtual std::string TraceCaption();

};

class TArxTypeBIn : public TArxMove
{
private:
    std::map<int, Dates::DateTime_t> tlg_ids;
    void readTlgIds(const Dates::DateTime_t& arx_date, size_t max_rows);
public:
    TArxTypeBIn(const Dates::DateTime_t& utc_date);
    virtual ~TArxTypeBIn() = default;
    virtual bool Next(size_t max_rows, int duration);
    virtual std::string TraceCaption();

};

class TArxTlgTrips : public TArxMove
{
  private:
    int point_ids_count;
    std::vector<PointIdTlg_t> tlg_trip_points;
    void readTlgTripPoints(const Dates::DateTime_t &arx_date, size_t max_rows);
  public:
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
    virtual ~TArxMoveNoFlt() = default;
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

}//namespace PG_ARX

#endif // ARX_DAILY_PG_H
