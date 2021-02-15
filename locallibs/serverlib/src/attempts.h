#pragma once

#include <string>
#include <functional>
#include "pg_cursctl.h"

namespace ServerFramework
{

class NewDaemon;

// state of request to be processed
enum class AttemptState {
    Fine,                   // <- normal state - work as always
    LastAttempt,            // <- last attempt - try to raise log level and work with full logs
    AllAttemptsExhausted,   // <- last attempts failed - try to handle this
    TotallyBroken,          // <- handling of last attempt failed trace error and completele skip this request
};

AttemptState makeAttemptState(unsigned attempts);

class AttemptsCounter
{
public:
    static AttemptsCounter& getCounter(const std::string& type);

    virtual ~AttemptsCounter() {}
    virtual AttemptState makeAttempt(const std::string& id) = 0;
    virtual void undoAttempt(const std::string& id) = 0;
    virtual void dropAttempt(const std::string& id) = 0;

    const std::string& type() const;
    void clear();

#ifdef XP_TESTING
    static void useDb(bool f);
#endif // XP_TESTING
protected:
    AttemptsCounter(const std::string& type, std::function<void ()> fClear);

private:
    const std::string type_;
    std::function<void ()> fClear_;

    friend class ClearProcAttemptsTask;
};

void setupAttemptCounter(const std::string& type, std::function<void ()> funcClear);
void setupAttemptCounter(const std::string& type, std::function<void ()> funcClear, PgCpp::SessionDescriptor);

void setupAttemptsCleanerTask(NewDaemon&, unsigned freqInSeconds);

} // ServerFramework
