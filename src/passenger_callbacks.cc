#include "passenger_callbacks.h"
#include "passenger.h"
#include "alarms.h"
#include "counters.h"
#include "base_callbacks.h"

#define STDLOG NICKNAME,__FILE__,__LINE__
#define NICKNAME "VLAD"
#include "serverlib/slogger.h"

class PaxRemCallbacks: public PaxEventCallbacks<TRemCategory>
{
  public:
    void onChange(TRACE_SIGNATURE,
                  const PaxOrigin& paxOrigin,
                  const TRemCategory& category,
                  const std::set<PaxIdWithSegmentPair>& paxIds);

    void onChangeASVC(const PaxOrigin& paxOrigin,
                      const std::set<PaxIdWithSegmentPair>& paxIds);
    void onChangePDRem(const PaxOrigin& paxOrigin,
                       const std::set<PaxIdWithSegmentPair>& paxIds);
};

class PaxChangesCallbacks: public PaxEventCallbacks<PaxChanges>
{
  public:
    void onChange(TRACE_SIGNATURE,
                  const PaxOrigin& paxOrigin,
                  const PaxChanges& category,
                  const std::set<PaxIdWithSegmentPair>& paxIds);
};

static void checkCustomAlarms(const PaxOrigin& paxOrigin,
                              const std::set<PaxIdWithSegmentPair>& paxIds)
{
  if (paxOrigin==paxCheckIn)
  {
    for(const PaxIdWithSegmentPair& paxId : paxIds)
      TPaxAlarmHook::set(Alarm::SyncCustomAlarms, paxId().get());
  }
}

static bool SyncPaxASVC(int pax_id)
{
  bool result=false;
  if (CheckIn::DeletePaxASVC(pax_id)) result=true;
  if (CheckIn::AddPaxASVC(pax_id, false)) result=true;
  if (result)
  {
      addAlarmByPaxId(pax_id, {Alarm::SyncEmds}, {paxCheckIn});
      TPaxAlarmHook::set(Alarm::UnboundEMD, pax_id);
      TPaxAlarmHook::set(Alarm::SyncCustomAlarms, pax_id);
  }
  return result;
}

static bool SyncPaxPD(int pax_id)
{
  bool result=false;
  if (CheckIn::DeletePaxPD(pax_id)) result=true;
  if (CheckIn::AddPaxPD(pax_id, false)) result=true;
  return result;
}

void PaxRemCallbacks::onChangeASVC(const PaxOrigin& paxOrigin,
                                   const std::set<PaxIdWithSegmentPair>& paxIds)
{
  if (paxOrigin!=paxPnl) return;

  Timing::Points timing(__func__);
  timing.start("SyncPaxASVC");
  for(const PaxIdWithSegmentPair& paxId : paxIds) SyncPaxASVC(paxId().get());
  timing.finish("SyncPaxASVC");
}

void PaxRemCallbacks::onChangePDRem(const PaxOrigin& paxOrigin,
                                    const std::set<PaxIdWithSegmentPair>& paxIds)
{
  if (paxOrigin!=paxPnl) return;

  Timing::Points timing(__func__);
  timing.start("SyncPaxPD");
  for(const PaxIdWithSegmentPair& paxId : paxIds) SyncPaxPD(paxId().get());
  timing.finish("SyncPaxPD");
}

void PaxRemCallbacks::onChange(TRACE_SIGNATURE,
                               const PaxOrigin& paxOrigin,
                               const TRemCategory& category,
                               const std::set<PaxIdWithSegmentPair>& paxIds)
{
  switch(category)
  {
    case remTKN:
      checkCustomAlarms(paxOrigin, paxIds);
      break;
    case remFQT:
      checkCustomAlarms(paxOrigin, paxIds);
      break;
    case remASVC:
      onChangeASVC(paxOrigin, paxIds);
      break;
    case remPD:
      onChangePDRem(paxOrigin, paxIds);
      break;
    default: break;
  }
}

void PaxChangesCallbacks::onChange(TRACE_SIGNATURE,
                                   const PaxOrigin& paxOrigin,
                                   const PaxChanges& category,
                                   const std::set<PaxIdWithSegmentPair>& paxIds)
{
  switch(category)
  {
    case PaxChanges::New:
      checkCustomAlarms(paxOrigin, paxIds);
      break;
    default: break;
  }
}

void initPassengerCallbacks()
{
  static bool init=false;
  if (init) return;

  CallbacksSuite< PaxEventCallbacks<TRemCategory> >::Instance()->addCallbacks(new PaxRemCallbacks);
  CallbacksSuite< PaxEventCallbacks<PaxChanges> >::Instance()->addCallbacks(new PaxChangesCallbacks);
  init=true;
}
