//---------------------------------------------------------------------------
#ifndef _ARX_DAILY_H_
#define _ARX_DAILY_H_

#include "date_time.h"
#include "oralib.h"

using BASIC::date_time::TDateTime;

int ARX_MIN_DAYS();
int ARX_MAX_DAYS();
int ARX_DURATION();
int ARX_SLEEP();
int ARX_MAX_ROWS();

bool arx_daily( TDateTime utcdate );

class TArxMove
{
  protected:
    int proc_count;
    TDateTime utcdate;
    TQuery *Qry;
  public:
    TArxMove(TDateTime utc_date);
    virtual ~TArxMove();
    virtual void BeforeProc() { proc_count=0; };
    virtual void AfterProc() { Qry->Close(); };
    virtual bool Next(int max_rows, int duration) = 0; //duration - длительность итерации в сек.
    virtual std::string TraceCaption() = 0;
    int Processed() { return proc_count; };
};

class TArxMoveFlt : public TArxMove
{
  private:
    int step, move_ids_count;
    std::map<int,TDateTime> move_ids;
  protected:
    TQuery *PointsQry;
    bool GetPartKey(int move_id, TDateTime& part_key, double &date_range);
    void LockAndCollectStat(int move_id);
  public:
    TArxMoveFlt(TDateTime utc_date);
    virtual ~TArxMoveFlt();
    virtual void AfterProc();
    virtual bool Next(int max_rows, int duration);
    virtual std::string TraceCaption();

};

class TArxTypeBIn : public TArxMove
{
  private:
    int step, tlg_ids_count;
    std::map<int,TDateTime> tlg_ids;
    TQuery *TlgQry;
    bool CheckTlgId(int tlg_id);
  public:
    TArxTypeBIn(TDateTime utc_date);
    virtual ~TArxTypeBIn();
    virtual void AfterProc();
    virtual bool Next(int max_rows, int duration);
    virtual std::string TraceCaption();

};

class TArxTlgTrips : public TArxMove
{
  private:
    int step, point_ids_count;
    std::vector<int> point_ids;
  public:
    TArxTlgTrips(TDateTime utc_date);
    virtual void AfterProc();
    virtual bool Next(int max_rows, int duration);
    virtual std::string TraceCaption();
};

class TArxMoveNoFlt : public TArxMove
{
  protected:
    int step;
  public:
    TArxMoveNoFlt(TDateTime utc_date);
    virtual ~TArxMoveNoFlt();
    virtual bool Next(int max_rows, int duration);
    virtual std::string TraceCaption();
};

class TArxNormsRatesEtc : public TArxMoveNoFlt
{
  public:
    TArxNormsRatesEtc(TDateTime utc_date);
    virtual std::string TraceCaption();
};

class TArxTlgsFilesEtc : public TArxMoveNoFlt
{
  public:
    TArxTlgsFilesEtc(TDateTime utc_date);
    virtual std::string TraceCaption();
};


#endif
