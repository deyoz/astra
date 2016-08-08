//---------------------------------------------------------------------------
#include "arx_daily.h"
#include "oralib.h"
#include "exceptions.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "qrys.h"
#include "stat.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include "serverlib/test.h"

using namespace ASTRA;
using namespace BASIC::date_time;
using namespace EXCEPTIONS;
using namespace std;

int ARX_MIN_DAYS()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ARX_MIN_DAYS",15,NoExists,NoExists);
  return VAR;
};

int ARX_MAX_DAYS()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ARX_MAX_DAYS",15,NoExists,NoExists);
  return VAR;
};

int ARX_DURATION()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ARX_DURATION",1,60,15);
  return VAR;
};

int ARX_SLEEP()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ARX_SLEEP",1,NoExists,60);
  return VAR;
};

int ARX_MAX_ROWS()
{
  static int VAR=NoExists;
  if (VAR==NoExists)
    VAR=getTCLParam("ARX_MAX_ROWS",100,NoExists,1000);
  return VAR;
};

//============================= TArxMove =============================

TArxMove::TArxMove(TDateTime utc_date)
{
  proc_count=0;
  utcdate=utc_date;
  Qry = OraSession.CreateQuery();
};

TArxMove::~TArxMove()
{
  OraSession.DeleteQuery(*Qry);
};

//============================= TArxMoveFlt =============================

TArxMoveFlt::TArxMoveFlt(TDateTime utc_date):TArxMove(utc_date)
{
  step=0;
  move_ids_count=0;
  PointsQry = OraSession.CreateQuery();
  PointsQry->Clear();
  PointsQry->SQLText =
    "SELECT act_out,est_out,scd_out,act_in,est_in,scd_in,pr_del "
    "FROM points "
    "WHERE move_id=:move_id "
    "ORDER BY point_num";
  PointsQry->DeclareVariable("move_id",otInteger);
};

TArxMoveFlt::~TArxMoveFlt()
{
  OraSession.DeleteQuery(*PointsQry);
};

bool TArxMoveFlt::GetPartKey(int move_id, TDateTime& part_key, double &date_range)
{
  part_key=NoExists;
  date_range=NoExists;

  PointsQry->SetVariable("move_id",move_id);
  PointsQry->Execute();

  TDateTime first_date=NoExists;
  TDateTime last_date=NoExists;
  TDateTime max_time_out=NoExists;
  TDateTime min_time_out=NoExists;
  TDateTime max_time_in=NoExists;
  TDateTime min_time_in=NoExists;
  TDateTime final_act_in=NoExists;
  TDateTime max_time=NoExists;
  bool deleted=true;

  while(!PointsQry->Eof)
  {
    if (PointsQry->FieldAsInteger("pr_del")!=-1) deleted=false;

    {
      max_time_out=NoExists;
      min_time_out=NoExists;
      for(int i=0;i<=2;i++)
      {
        int idx;
        switch(i)
        {
          case 0: idx=PointsQry->FieldIndex("scd_out");
                  break;
          case 1: idx=PointsQry->FieldIndex("est_out");
                  break;
         default: idx=PointsQry->FieldIndex("act_out");
                  break;
        };
        if (PointsQry->FieldIsNULL(idx)) continue;
        if (max_time_out==NoExists || max_time_out<PointsQry->FieldAsDateTime(idx))
          max_time_out=PointsQry->FieldAsDateTime(idx);
        if (min_time_out==NoExists || min_time_out>PointsQry->FieldAsDateTime(idx))
          min_time_out=PointsQry->FieldAsDateTime(idx);
      };

      if (max_time_out!=NoExists &&
          (max_time==NoExists || max_time<max_time_out))
        max_time=max_time_out;

      if (PointsQry->FieldAsInteger("pr_del")!=-1)
      {
        if (max_time_out!=NoExists &&
            (last_date==NoExists || last_date<max_time_out))
          last_date=max_time_out;
        if (min_time_out!=NoExists &&
            (first_date==NoExists || first_date>min_time_out))
          first_date=min_time_out;
      };
    };

    PointsQry->Next();

    if (PointsQry->Eof) break;

    {
      max_time_in=NoExists;
      min_time_in=NoExists;
      for(int i=0;i<=2;i++)
      {
        int idx;
        switch(i)
        {
          case 0: idx=PointsQry->FieldIndex("scd_in");
                  break;
          case 1: idx=PointsQry->FieldIndex("est_in");
                  break;
         default: idx=PointsQry->FieldIndex("act_in");
                  break;
        };
        if (PointsQry->FieldIsNULL(idx)) continue;
        if (max_time_in==NoExists || max_time_in<PointsQry->FieldAsDateTime(idx))
          max_time_in=PointsQry->FieldAsDateTime(idx);
        if (min_time_in==NoExists || min_time_in>PointsQry->FieldAsDateTime(idx))
          min_time_in=PointsQry->FieldAsDateTime(idx);
      };

      if (max_time_in!=NoExists &&
          (max_time==NoExists || max_time<max_time_in))
        max_time=max_time_in;

      if (PointsQry->FieldAsInteger("pr_del")!=-1)
      {
        if (max_time_in!=NoExists &&
            (last_date==NoExists || last_date<max_time_in))
          last_date=max_time_in;
        if (min_time_in!=NoExists &&
            (first_date==NoExists || first_date>min_time_in))
          first_date=min_time_in;

        if (PointsQry->FieldAsInteger("pr_del")==0)
        {
       	  if (!PointsQry->FieldIsNULL("act_in"))
            final_act_in=PointsQry->FieldAsDateTime("act_in");
          else
            final_act_in=NoExists;
        };
      };
    };
  };
  if (!deleted)
  {
    if (first_date!=NoExists && last_date!=NoExists)
    {

      if ( (final_act_in!=NoExists && last_date<utcdate-ARX_MIN_DAYS()) ||
           (final_act_in==NoExists && last_date<utcdate-ARX_MAX_DAYS()) )
      {
        //��६����� � ��娢
        part_key=last_date;
        date_range=last_date-first_date;
        return true;
      };
    };
  }
  else
  {
    //��������� 㤠����� ३�
    if (max_time==NoExists || max_time<utcdate-ARX_MIN_DAYS())
    {
      //㤠����
      part_key=NoExists;
      date_range=NoExists;
      return true;
    };
  };
  return false;
};

void TArxMoveFlt::LockAndCollectStat(int move_id)
{
  QParams PointsQryParams;
  PointsQryParams << QParam("move_id", otInteger, move_id);
  TCachedQuery PointsQry("SELECT point_id,pr_del,pr_reg FROM points WHERE move_id=:move_id FOR UPDATE", PointsQryParams);
  TQuery &Qry=PointsQry.get();
  Qry.Execute();
  for(;!Qry.Eof;Qry.Next())
  {
    int pr_del=Qry.FieldAsInteger("pr_del");
    bool pr_reg=Qry.FieldAsInteger("pr_reg")!=0;
    int point_id=Qry.FieldAsInteger("point_id");
    if (pr_del<0) continue;
    if (pr_del==0 && pr_reg) get_flight_stat(point_id, true);
    TReqInfo::Instance()->LocaleToLog("EVT.FLIGHT_MOOVED_TO_ARCHIVE", evtFlt, point_id);
  };
};

bool TArxMoveFlt::Next(int max_rows, int duration)
{
  if (step==0 || step==3 || step==6)
  {
    if (Qry->SQLText.IsEmpty())
    {
      Qry->Clear();
      switch (step)
      {
        case 0:
          Qry->SQLText=
            "SELECT move_id FROM points "
            "WHERE time_in > TO_DATE('01.01.0001','DD.MM.YYYY') AND time_in<:arx_date ";
          break;
        case 3:
          Qry->SQLText=
            "SELECT move_id FROM points "
            "WHERE time_out > TO_DATE('01.01.0001','DD.MM.YYYY') AND time_out<:arx_date ";
          break;
        case 6:
          Qry->SQLText=
            "SELECT move_id FROM points "
            "WHERE time_in  = TO_DATE('01.01.0001','DD.MM.YYYY') AND "
            "      time_out = TO_DATE('01.01.0001','DD.MM.YYYY') ";
          break;
      };
      if (step!=6)
        Qry->CreateVariable("arx_date",otDate,utcdate-ARX_MIN_DAYS());
      Qry->Execute();
    };

    for(;!Qry->Eof;Qry->Next())
    {
      int move_id=Qry->FieldAsInteger("move_id");

      if (move_ids.find(move_id)!=move_ids.end()) continue;

      TDateTime part_key;
      double date_range;

      if (GetPartKey(move_id,part_key,date_range))
      {
        move_ids[move_id]=part_key;
        move_ids_count++;
        Qry->Next();
        if (move_ids_count<max_rows)
          return true;
        else
          break;
      };
    };
    if (!Qry->Eof)
      step+=1;
    else
      step+=2;
    Qry->Clear();
    return true;
  };

  if (step==1 || step==4 || step==7 ||
      step==2 || step==5 || step==8)
  {
    if (Qry->SQLText.IsEmpty())
    {
      Qry->Clear();
      Qry->SQLText =
        "BEGIN "
        "  arch.move(:move_id,:part_key,:date_range); "
        "END;";
      Qry->DeclareVariable("move_id",otInteger);
      Qry->DeclareVariable("part_key",otDate);
      Qry->DeclareVariable("date_range",otInteger);
    };
    while (!move_ids.empty())
    {
      int move_id=move_ids.begin()->first;
      move_ids.erase(move_ids.begin());
      move_ids_count--;

      TDateTime part_key;
      double date_range;

      if (GetPartKey(move_id,part_key,date_range))
      {
        try
        {
          //� ��娢
          Qry->SetVariable("move_id",move_id);
          if (part_key!=NoExists)
          	Qry->SetVariable("part_key",part_key);
          else
          	Qry->SetVariable("part_key",FNull);
          if (date_range!=NoExists)
          {
            if (date_range<0) throw Exception("date_range=%f", date_range);
            if (date_range<1)
              Qry->SetVariable("date_range",FNull);
            else
            {
              int date_range_int=(int)ceil(date_range);
              if (date_range_int>999) throw Exception("date_range_int=%d", date_range_int);
            	Qry->SetVariable("date_range",date_range_int);
            };
          }
          else Qry->SetVariable("date_range",FNull);
          LockAndCollectStat(move_id);
          Qry->Execute();
          OraSession.Commit();
          proc_count++;
        }
        catch(...)
        {
          if (part_key!=NoExists)
            ProgError( STDLOG, "move_id=%d, part_key=%s", move_id, DateTimeToStr( part_key, "dd.mm.yy" ).c_str() );
          else
            ProgError( STDLOG, "move_id=%d, part_key=NoExists", move_id );
          throw;
        };
        return true;
      };
    };
    if (step==1 || step==4 || step==7)
      step--;
    else
      step++;
    Qry->Clear();
    return true;
  };
  return false;
};

void TArxMoveFlt::AfterProc()
{
  TArxMove::AfterProc();
  PointsQry->Close();
  Qry->Clear();  //��易⥫쭮, ���� � ᫥� ࠧ �� �ࠡ�⠥� Execute
};

string TArxMoveFlt::TraceCaption()
{
  return "TArxMoveFlt";
};

//============================= TArxTypeBIn =============================

TArxTypeBIn::TArxTypeBIn(TDateTime utc_date):TArxMove(utc_date)
{
  step=0;
  tlg_ids_count=0;
  TlgQry = OraSession.CreateQuery();
  TlgQry->Clear();
  TlgQry->SQLText =
    "SELECT time_receive FROM tlgs_in "
    "WHERE id=:id AND time_receive>=:arx_date AND rownum<2";
  TlgQry->DeclareVariable("id",otInteger);
  TlgQry->CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS());
};

TArxTypeBIn::~TArxTypeBIn()
{
  OraSession.DeleteQuery(*TlgQry);
};

bool TArxTypeBIn::CheckTlgId(int tlg_id)
{
  TlgQry->SetVariable("id",tlg_id);
  TlgQry->Execute();
  return TlgQry->Eof;
};

bool TArxTypeBIn::Next(int max_rows, int duration)
{
  if (step==0)
  {
    if (Qry->SQLText.IsEmpty())
    {
      Qry->Clear();
      Qry->SQLText=
        "SELECT id,time_receive "
        "FROM tlgs_in "
        "WHERE time_receive<:arx_date AND "
        "      NOT EXISTS(SELECT * FROM tlg_source WHERE tlg_source.tlg_id=tlgs_in.id AND rownum<2) AND "
        "      NOT EXISTS(SELECT * FROM tlgs_in a WHERE a.id=tlgs_in.id AND time_receive>=:arx_date AND rownum<2)";
      Qry->CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS());
      Qry->Execute();
    };

    for(;!Qry->Eof;Qry->Next())
    {
      int tlg_id=Qry->FieldAsInteger("id");

      if (tlg_ids.find(tlg_id)!=tlg_ids.end()) continue;

  //    if (CheckTlgId(tlg_id))
      {
        tlg_ids[tlg_id]=Qry->FieldAsDateTime("time_receive");
        tlg_ids_count++;
        Qry->Next();
        if (tlg_ids_count<max_rows)
          return true;
        else
          break;
      };
    };
    if (!Qry->Eof)
      step+=1;
    else
      step+=2;
    Qry->Clear();
    return true;
  };

  if (step==1 || step==2)
  {
    if (Qry->SQLText.IsEmpty())
    {
      Qry->Clear();
      Qry->SQLText =
      "BEGIN "
      "  arch.move_typeb_in(:tlg_id); "
      "END;";
      Qry->DeclareVariable("tlg_id",otInteger);
    };
    while (!tlg_ids.empty())
    {
      int tlg_id=tlg_ids.begin()->first;
      tlg_ids.erase(tlg_ids.begin());
      tlg_ids_count--;

 //     if (CheckTlgId(tlg_id)) ����୮� ��ன ࠧ �� ��易⥫쭮 �஢�����
      {
        try
        {
          //� ��娢
          Qry->SetVariable("tlg_id",tlg_id);
          Qry->Execute();
          OraSession.Commit();
          proc_count++;
        }
        catch(...)
        {
          ProgError( STDLOG, "typeb_in.id=%d", tlg_id );
          throw;
        };
        return true;
      };
    };
    if (step==1)
      step--;
    else
      step++;
    Qry->Clear();
    return true;
  };
  return false;
};

void TArxTypeBIn::AfterProc()
{
  TArxMove::AfterProc();
  TlgQry->Close();
  Qry->Clear();  //��易⥫쭮, ���� � ᫥� ࠧ �� �ࠡ�⠥� Execute
};

string TArxTypeBIn::TraceCaption()
{
  return "TArxTypeBIn";
};

//============================= TArxTlgTrips =============================

TArxTlgTrips::TArxTlgTrips(TDateTime utc_date):TArxMove(utc_date)
{
  step=0;
  point_ids_count=0;
};

bool TArxTlgTrips::Next(int max_rows, int duration)
{
  if (step==0)
  {
    if (Qry->SQLText.IsEmpty())
    {
      Qry->Clear();
      Qry->SQLText =
          "SELECT point_id "
          "FROM tlg_trips,tlg_binding "
          "WHERE tlg_trips.point_id=tlg_binding.point_id_tlg(+) AND tlg_binding.point_id_tlg IS NULL AND "
          "      tlg_trips.scd<:arx_date";
      Qry->CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS());
      Qry->Execute();
    };

    for(;!Qry->Eof;Qry->Next())
    {
      int point_id=Qry->FieldAsInteger("point_id");

      point_ids.push_back(point_id);
      point_ids_count++;
      Qry->Next();
      if (point_ids_count<max_rows)
        return true;
      else
        break;
    };
    if (!Qry->Eof)
      step+=1;
    else
      step+=2;
    Qry->Clear();
    return true;
  };

  if (step==1 || step==2)
  {
    if (Qry->SQLText.IsEmpty())
    {
      Qry->Clear();
      Qry->SQLText=
        "BEGIN "
        "  arch.tlg_trip(:point_id); "
        "END;";
      Qry->DeclareVariable("point_id",otInteger);
    };
    while (!point_ids.empty())
    {
      int point_id=*(point_ids.begin());
      point_ids.erase(point_ids.begin());
      point_ids_count--;

      try
      {
        //� ��娢
        Qry->SetVariable("point_id",point_id);
        Qry->Execute();
        OraSession.Commit();
        proc_count++;
      }
      catch(...)
      {
        ProgError( STDLOG, "tlg_trips.point_id=%d", point_id );
        throw;
      };
      return true;
    };
    if (step==1)
      step--;
    else
      step++;
    Qry->Clear();
    return true;
  };
  return false;
};

void TArxTlgTrips::AfterProc()
{
  TArxMove::AfterProc();
  Qry->Clear();  //��易⥫쭮, ���� � ᫥� ࠧ �� �ࠡ�⠥� Execute
};

string TArxTlgTrips::TraceCaption()
{
  return "TArxTlgTrips";
};

//============================= TArxMoveNoFlt =============================

TArxMoveNoFlt::TArxMoveNoFlt(TDateTime utc_date):TArxMove(utc_date)
{
  step=0;
  Qry->Clear();
  Qry->SQLText=
    "BEGIN "
    "  arch.move(:arx_date,:max_rows,:time_duration,:step); "
    "END;";
  Qry->CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS());
  Qry->DeclareVariable("max_rows",otInteger);
  Qry->DeclareVariable("time_duration",otInteger);
  Qry->DeclareVariable("step",otInteger);
};

TArxMoveNoFlt::~TArxMoveNoFlt()
{
  //
};

bool TArxMoveNoFlt::Next(int max_rows, int duration)
{
  if (step>0)
    Qry->SetVariable("step",step);
  else
    Qry->SetVariable("step",1);
  Qry->SetVariable("max_rows",max_rows);
  Qry->SetVariable("time_duration",duration);
  Qry->Execute();
  OraSession.Commit();
  proc_count++;
  step=Qry->GetVariableAsInteger("step");
  return step>0;
};

string TArxMoveNoFlt::TraceCaption()
{
  return "TArxMoveNoFlt";
};

//============================= TArxNormsRatesEtc =============================

TArxNormsRatesEtc::TArxNormsRatesEtc(TDateTime utc_date):TArxMoveNoFlt(utc_date)
{
  step=0;
  Qry->Clear();
  Qry->SQLText=
    "BEGIN "
    "  arch.norms_rates_etc(:arx_date,:max_rows,:time_duration,:step); "
    "END;";
  Qry->CreateVariable("arx_date",otDate,utcdate-ARX_MAX_DAYS()-15);
  Qry->DeclareVariable("max_rows",otInteger);
  Qry->DeclareVariable("time_duration",otInteger);
  Qry->DeclareVariable("step",otInteger);
};

string TArxNormsRatesEtc::TraceCaption()
{
  return "TArxNormsRatesEtc";
};

//============================= TArxTlgsFilesEtc===========

TArxTlgsFilesEtc::TArxTlgsFilesEtc(TDateTime utc_date):TArxMoveNoFlt(utc_date)
{
  step=0;
  Qry->Clear();
  Qry->SQLText=
    "BEGIN "
    "  arch.tlgs_files_etc(:arx_date,:max_rows,:time_duration,:step); "
    "END;";
  Qry->CreateVariable("arx_date",otDate,utcdate-ARX_MIN_DAYS());
  Qry->DeclareVariable("max_rows",otInteger);
  Qry->DeclareVariable("time_duration",otInteger);
  Qry->DeclareVariable("step",otInteger);
};

string TArxTlgsFilesEtc::TraceCaption()
{
  return "TArxTlgsFilesEtc";
};


bool arx_daily(TDateTime utcdate)
{
  modf(utcdate,&utcdate);
  static TDateTime prior_utcdate=NoExists;
  static time_t prior_exec=0;
  static int step=1;
  static TArxMove* arxMove=NULL;

  if (time(NULL)-prior_exec<ARX_SLEEP()) return false;

  time_t time_finish=time(NULL)+ARX_DURATION();

	if (prior_utcdate!=utcdate)
	{
	  if (arxMove!=NULL)
	  {
	    delete arxMove;
	    arxMove=NULL;
	  };
	  step=1;
	  prior_utcdate=utcdate;
	  ProgTrace(TRACE5,"arx_daily START");
	};

	for(;step<7;step++)
	{
	  if (arxMove==NULL)
	  {
	    switch (step)
	    {
	      case 1: arxMove = new TArxMoveFlt(utcdate);
	              break;
	      case 2: arxMove = new TArxMoveNoFlt(utcdate);
	              break;
	      case 3: arxMove = new TArxTlgTrips(utcdate);
	              break;
	      case 4: arxMove = new TArxTypeBIn(utcdate);
	              break;
	      case 5: arxMove = new TArxNormsRatesEtc(utcdate);
	              break;
	      case 6: arxMove = new TArxTlgsFilesEtc(utcdate);
	              break;
	    };
	    ProgTrace(TRACE5,"arx_daily: %s started",arxMove->TraceCaption().c_str());
	  }
	  else
	  {
	    ProgTrace(TRACE5,"arx_daily: %s continued",arxMove->TraceCaption().c_str());
	  };

	  arxMove->BeforeProc();

    try
    {
      int duration;
      do
      {
        duration=time_finish-time(NULL);
        if (duration<=0)
  	    {
  	      ProgTrace(TRACE5,"arx_daily: %d iterations processed",arxMove->Processed());
  	      arxMove->AfterProc();
  	      prior_exec=time(NULL);
  	      return false;
  	    };
      }
      while(arxMove->Next(ARX_MAX_ROWS(),duration));

      ProgTrace(TRACE5,"arx_daily: %d iterations processed",arxMove->Processed());
	    arxMove->AfterProc();
    }
    catch(...)
    {
      ProgTrace(TRACE5,"arx_daily: %d iterations processed",arxMove->Processed());
      arxMove->AfterProc();
      prior_exec=time(NULL);
      throw;
    };

    ProgTrace(TRACE5,"arx_daily: %s finished",arxMove->TraceCaption().c_str());

	  delete arxMove;
	  arxMove=NULL;
	};

  ProgTrace(TRACE5,"arx_daily FINISH");
  prior_exec=time(NULL);
  return true;
};


