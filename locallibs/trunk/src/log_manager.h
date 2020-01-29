#ifndef __LOG_MANAGER__
#define __LOG_MANAGER__

#include <string>
#include <memory>

// LogManager allows to switch log to modes: MaxLog (all log messages)
// or MinLog (TRACE1 and lower log messages) on the specified object
// (such as a PNR or a telegram).
//
// LogManager is controlled by the sets:
//
//    - the set of "min log" objects.
//    - the set of "max log" objects.
//      Objects have limited retension time.
//
//    - the set of "turned on" objects (log mode for an object is turned on).
//
// The "real" log mode is turned on if the set intersection between
// the "max log" or "min log" objects and the "turned on" objects is not empty.
// If there are both intersections log mode switches to MaxLog.
class LogManager
{
public:
    typedef std::string ObjectType;
    typedef std::string ObjectId;
    enum class Mode {MinLog, MaxLog};

    // Get the singleton
    static LogManager& Instance();

    // Returns true if the set intesection between the "max log" objects
    // and the "turned on" objects is not empty.
    bool isWriteLog();

    // To write, or not to write.
    bool isWriteLog(int level);

    // Add the specified object to the set of "min log" or "max log" objects.
    // Retension time is limited by LOG_MANAGER(LOG_TIME) tcl variable.
    void load(const ObjectType& type, const ObjectId& id,
        Mode mode = Mode::MaxLog);
    // Remove the specified object from the set of "min log" or
    // "max log" objects. Returns if it has actually been unloaded.
    bool unload(const ObjectType& type, const ObjectId& id,
        Mode mode = Mode::MaxLog);

    // Add the specified object to the set of "turned on" objects.
    // Retension time is infinite.
    void start(const ObjectType& type, const ObjectId& id);
    // Remove the specified object from the set of "turned on" objects.
    void stop(const ObjectType& type, const ObjectId& id);
private:
    explicit LogManager();
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;
    struct Data;
    std::shared_ptr<Data> data;
};


class LogLevelHolder
{
public:
    explicit LogLevelHolder():
        started_(false)
    {
    }

    LogLevelHolder(const LogManager::ObjectType& type,
        const LogManager::ObjectId& id): started_(false)
    {
        reset(type, id);
    }

    void reset(const LogManager::ObjectType& type,
        const LogManager::ObjectId& id)
    {
        type_ = type;
        id_ = id;
        LogManager::Instance().start(type_, id_);
        started_ = true;
    }

    void release()
    {
        if (started_) {
            LogManager::Instance().stop(type_, id_);
            started_ = false;
        }
    }

    ~LogLevelHolder()
    {
        release();
    }
private:
    LogManager::ObjectType type_;
    LogManager::ObjectId id_;
    bool started_;
};


class LogPultHolder
{
public:
    explicit LogPultHolder():id_(), mode_(LogManager::Mode::MaxLog),
        needUnload_(false)
    {
    }

    explicit LogPultHolder(const LogManager::ObjectId& i,
        LogManager::Mode m = LogManager::Mode::MaxLog):
        id_(i), mode_(m), needUnload_(false)
    {
        LogManager::Instance().load("pult", id_, mode_);
        needUnload_ = true;
    }

    LogPultHolder(LogPultHolder& other)
        :id_(other.id_), mode_(other.mode_), needUnload_(other.needUnload_)
    {
        other.needUnload_ = false;
    }

    LogPultHolder& operator=(LogPultHolder& rhs)
    {
        LogPultHolder tmp(rhs);

        this->swap(tmp);

        return *this;
    }

    void swap(LogPultHolder& rhs)
    {
        this->id_.swap(rhs.id_);
        std::swap(this->mode_,rhs.mode_);
        std::swap(this->needUnload_,rhs.needUnload_);
    }

    void reset(const LogManager::ObjectId& i,
        LogManager::Mode m = LogManager::Mode::MaxLog)
    {
        LogPultHolder tmp(i, m);

        this->swap(tmp);
    }

    void release()
    {
       if (needUnload_) {
            LogManager::Instance().unload("pult", id_, mode_);
            needUnload_ = false;
        }
    }

    ~LogPultHolder()
    {
        release();
    }

private:
    LogManager::ObjectId id_;
    LogManager::Mode mode_;
    bool needUnload_;
};

#endif
