#pragma once

#include <edilib/EdiSessionId_t.h>
#include "trip_tasks.h"

class PostponeTripTaskHandling
{
protected:
    static void insertDb(const TTripTaskKey& task, edilib::EdiSessionId_t sessId);
    static boost::optional<TTripTaskKey> deleteDb(edilib::EdiSessionId_t sessId);
public:
    static void postpone(const TTripTaskKey &task, edilib::EdiSessionId_t sessId);
    static boost::optional<TTripTaskKey> deleteWaiting(edilib::EdiSessionId_t sessId);
    static boost::optional<TTripTaskKey> findWaiting(edilib::EdiSessionId_t sessId);
    static bool copyWaiting(edilib::EdiSessionId_t srcSessId, edilib::EdiSessionId_t destSessId);
    static void deleteWaiting(const TTripTaskKey &task);
};

