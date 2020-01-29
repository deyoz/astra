#include "log_manager.h"
#include <cstdlib>
#include <ctime>
#include <map>
#include <set>
#include "lwriter.h"
#include "tcl_utils.h"

namespace {
    struct LogObject {
        LogManager::ObjectType type;
        LogManager::ObjectId id;
    };

    static bool operator<(const LogObject& a, const LogObject& b)
    {
        return std::make_pair(a.type, a.id) < std::make_pair(b.type, b.id);
    }

    bool isIntersection(
        const std::map<LogObject, time_t> &temporaryObjects,
        const std::multiset<LogObject> &turnedOnObjects)
    {
        // Most of the time temporaryObjects is empty,
        // so there is no significant performance penalty
        for (const auto& to : temporaryObjects) {
            if (turnedOnObjects.count(to.first))
                return true;
        }
        return false;
    }
} /* unnamed namespace */

struct LogManager::Data {
    std::map<LogObject, time_t> minLogObjects; // time_t holds time of creation
    std::map<LogObject, time_t> maxLogObjects; // or time of update
    std::multiset<LogObject> turnedOnObjects;
};

LogManager::LogManager():
    data(new Data())
{}

LogManager& LogManager::Instance()
{
    static LogManager* const lm = new LogManager();
    return *lm;
}

bool LogManager::isWriteLog()
{
    return isIntersection(data->maxLogObjects, data->turnedOnObjects);
}

static void CleanupOutdatedObjects(
    std::map<LogObject, time_t>& temporaryObjects,
    time_t now,
    double retensionTimeSeconds)
{
    auto it = temporaryObjects.begin();
    while (it != temporaryObjects.end()) {
        const time_t creation_or_update_time = it->second;
        if (std::difftime(now, creation_or_update_time) > retensionTimeSeconds) {
            it = temporaryObjects.erase(it);
        } else {
            ++it;
        }
    }
}

bool LogManager::isWriteLog(int level)
{
    static const double MAX_LOG_TIME_SECOND =
        readIntFromTcl("LOG_MANAGER(LOG_TIME)", 300);
    static const int CALL_CNT_BETWEEN_CLEAN =
        readIntFromTcl("LOG_MANAGER(FREQ_CHECK)", 100);
    static int callCnt = CALL_CNT_BETWEEN_CLEAN;

    if(!--callCnt){
        CleanupOutdatedObjects(
            data->minLogObjects, std::time(nullptr), MAX_LOG_TIME_SECOND);
        CleanupOutdatedObjects(
            data->maxLogObjects, std::time(nullptr), MAX_LOG_TIME_SECOND);
        callCnt = CALL_CNT_BETWEEN_CLEAN;
    }

    if(isIntersection(data->maxLogObjects, data->turnedOnObjects))
        return true;
    else if(isIntersection(data->minLogObjects, data->turnedOnObjects))
        return level<=1/*TRACE1*/;
    else
        return !cutLog(level);
}

void LogManager::load(const ObjectType& type, const ObjectId& id, Mode mode)
{
    const LogObject obj{type, id};
    if(mode == Mode::MinLog)
        data->minLogObjects[obj] = std::time(nullptr);
    else // mode == Mode::MaxLog
        data->maxLogObjects[obj] = std::time(nullptr);
}

bool LogManager::unload(const ObjectType& type, const ObjectId& id, Mode mode)
{
    const LogObject obj{type, id};
    if(mode == Mode::MinLog)
        return data->minLogObjects.erase(obj) > 0;
    else // mode == Mode::MaxLog
        return data->maxLogObjects.erase(obj) > 0;
}

void LogManager::start(const ObjectType& type, const ObjectId& id)
{
    data->turnedOnObjects.insert(LogObject{type, id});
}

void LogManager::stop(const ObjectType& type, const ObjectId& id)
{
    auto it = data->turnedOnObjects.find(LogObject{type, id});
    if (it != data->turnedOnObjects.end())
        data->turnedOnObjects.erase(it);
}
