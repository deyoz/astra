//---------------------------------------------------------------------------
#ifndef _ARX_DAILY_H_
#define _ARX_DAILY_H_

#include "basic.h"
#include "oralib.h"

const int ARX_MIN_DAYS();
const int ARX_MAX_DAYS();
const int ARX_DURATION();
const int ARX_SLEEP();
const int ARX_MAX_ROWS();

bool arx_daily( BASIC::TDateTime utcdate );

class TArxMove
{
  protected:
    int proc_count;
    BASIC::TDateTime utcdate;
    TQuery *Qry;
  public:
    TArxMove(BASIC::TDateTime utc_date);
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
    std::map<int,BASIC::TDateTime> move_ids;
  protected:
    TQuery *PointsQry;
    bool GetPartKey(int move_id, BASIC::TDateTime& part_key, double &date_range);
    void LockAndCollectStat(int move_id);
  public:
    TArxMoveFlt(BASIC::TDateTime utc_date);
    virtual ~TArxMoveFlt();
    virtual void AfterProc();
    virtual bool Next(int max_rows, int duration);
    virtual std::string TraceCaption();

};

class TArxTypeBIn : public TArxMove
{
  private:
    int step, tlg_ids_count;
    std::map<int,BASIC::TDateTime> tlg_ids;
    TQuery *TlgQry;
    bool CheckTlgId(int tlg_id);
  public:
    TArxTypeBIn(BASIC::TDateTime utc_date);
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
    TArxTlgTrips(BASIC::TDateTime utc_date);
    virtual void AfterProc();
    virtual bool Next(int max_rows, int duration);
    virtual std::string TraceCaption();
};

class TArxMoveNoFlt : public TArxMove
{
  protected:
    int step;
  public:
    TArxMoveNoFlt(BASIC::TDateTime utc_date);
    virtual ~TArxMoveNoFlt();
    virtual bool Next(int max_rows, int duration);
    virtual std::string TraceCaption();
};

class TArxNormsRatesEtc : public TArxMoveNoFlt
{
  public:
    TArxNormsRatesEtc(BASIC::TDateTime utc_date);
    virtual std::string TraceCaption();
};

class TArxTlgsFilesEtc : public TArxMoveNoFlt
{
  public:
    TArxTlgsFilesEtc(BASIC::TDateTime utc_date);
    virtual std::string TraceCaption();
};


#endif
