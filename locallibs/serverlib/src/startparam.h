#ifndef _SERVERLIB_STARTPARAM_H_
#define _SERVERLIB_STARTPARAM_H_

#include <string>
#include <algorithm>
#include <set>
#include <map>
#include <stdexcept>
#include "tcl_utils.h"
#include "string_cast.h"

namespace Supervisor {

void log_trace_1(const char* msg);

class GroupDescription
{
public:
    enum PROCESS_LEVEL{LEVEL_A = 0, LEVEL_C = 1, LEVEL_OTHER = 2, LEVEL_PROFILE = 3};

public:
    GroupDescription(int handlersCount, const std::string& startLine, int priority, PROCESS_LEVEL pl);

    GroupDescription(const GroupDescription& other);

    GroupDescription& operator=(const GroupDescription& rhs);

    bool operator<(const GroupDescription& rhs)const {

        return this->priority_ < rhs.priority_;
    }

    int handlersCount() const {
        return handlersCount_;
    }

    void addToHandlersCount(int diff) const {
        handlersCount_ += diff;
    }

    const std::string& startLine() const {
        return startLine_;
    }

    int priority() const {
        return priority_;
    }

    static const std::string& execFileName(int pl) {
        switch (pl) {
        case LEVEL_A: {
            static const std::string a(readStringFromTcl("LEVEL_A_EXEC_FILE"));
            return a;
        }
        case LEVEL_C: {
            static const std::string c(readStringFromTcl("LEVEL_C_EXEC_FILE"));
            return c;
        }
        case LEVEL_OTHER: {
            static const std::string other(readStringFromTcl("OTHER_EXEC_FILE"));
            return other;
        }
        case LEVEL_PROFILE: {
            static const std::string profile(readStringFromTcl("PROFILE_EXEC_FILE"));
            return profile;
        }
        default:
            throw std::logic_error("Unknown PROCESS_LEVEL value");
        }
    }

    const std::string& execFileName() const {
        return execFileName(pl_);
    }


    PROCESS_LEVEL level() const {
        return pl_;
    }

    void swap(GroupDescription& other){
        std::swap(this->handlersCount_, other.handlersCount_);
        this->startLine_.swap(other.startLine_);
        std::swap(this->priority_, other.priority_);
        std::swap(this->pl_, other.pl_);
    }

private:
    mutable int handlersCount_;
    std::string startLine_;
    int priority_; //Default key for associative containers. See operator< (...)
    PROCESS_LEVEL pl_;
};

struct GroupDescriptionCompare
{
    bool operator()(const GroupDescription& lhs, const GroupDescription& rhs) const {
        return lhs.startLine() < rhs.startLine();
    }
};

class StartParam
{
public:
    typedef std::multiset<GroupDescription>::iterator iterator;

    static StartParam& Instance();

    void add(const GroupDescription& gd){
        curConfig_.insert(gd);
    }

    iterator begin(){
        return curConfig_.begin();
    }

    iterator end(){
        return curConfig_.end();
    }

    void resetConfig() {
        curConfig_.clear();
    }

    //Precondition:
    //    true == curConfig_.clear().(call resetConfig)
    //    oldConfig_ equal configChanges_
    template<typename StartFunc, typename StopFunc>
    int applyDiff(StartFunc start, StopFunc stop);

    template<typename StartFunc>
    int start (StartFunc f);
private:
    typedef std::map<GroupDescription, bool, GroupDescriptionCompare>::iterator OldConfigIterator;

    static const int START_LINE_SIZE = 255;

private:
    explicit StartParam() : curConfig_(), oldConfig_()
    {
    }

    template<typename StartFunc, typename StopFunc>
    int applyDiffStep(StartFunc start, StopFunc stop, iterator beg, iterator end);

    void fillOldConfig() {
        oldConfig_.clear();
        for(iterator i = curConfig_.begin(); i != curConfig_.end(); ++i) {
            oldConfig_.emplace(*i, false);
        }
    }
    int changeOldConfig(OldConfigIterator& it, int diff, int hasErrors) {

        if (!hasErrors) {
            it->first.addToHandlersCount(diff);
        }

        return hasErrors;
    }

    int changeOldConfig(const GroupDescription& newGrp, int hasErrors) {
        if (!hasErrors) {
            oldConfig_.insert(std::make_pair(newGrp, true));
        }

        return hasErrors;
    }

    int changeOldConfig(OldConfigIterator& it, int hasErrors) {
        if (!hasErrors) {
            oldConfig_.erase(it);
        }

        return hasErrors;
    }

    //no copy
    StartParam(const StartParam&);
    StartParam& operator=(const StartParam&);

private:
    static std::map<std::string, int> grpID;

    std::multiset<GroupDescription> curConfig_;
    std::map<GroupDescription, bool, GroupDescriptionCompare> oldConfig_;
};

template<typename StartFunc>
int StartParam::start(StartFunc f)
{
    if (curConfig_.empty()) {
        log_trace_1("StartParam: call Supervisor::Startparam::start() with empty groups list");
        return 1;
    }

    std::pair<iterator, iterator> range( curConfig_.equal_range(*curConfig_.begin())); //Диапазон групп с самым маленьким приоритетом
    std::string resultStartLine;
    const char* objv[1];
    int retValue = 0;
    int sleep_second = readIntFromTcl("OBRZAP_START_PAUSE", 1); //Время паузы между запусками групп с разными приоритетами

    fillOldConfig();
    resultStartLine.reserve(START_LINE_SIZE);
    while( range.second != curConfig_.end() ) { //До последней группы
        for(iterator i = range.first; i != range.second; ++i) { //Для каждого элемента с данным приоритетом
            for(int handler = 0; handler < i->handlersCount(); ++handler) {
                *objv = i->startLine().c_str();
                retValue |= f(1, objv, getTclInterpretator(), i->level());
            }
        }
        range = curConfig_.equal_range(*(range.second)); //Следующий диапазон групп с большим приоритетом
        sleep(sleep_second);
    }
    for(iterator i = range.first; i != range.second; ++i) {
        for(int handler = 0; handler < i->handlersCount(); ++handler) {
            *objv = i->startLine().c_str();
            retValue |= f(1, objv, getTclInterpretator(), i->level());
        }
    }

    return retValue; //Возвращаем битовый набор из того, что вернула нам f
}

template<typename StartFunc, typename StopFunc>
int StartParam::applyDiff(StartFunc start, StopFunc stop)
{
    if (curConfig_.empty()) {
        log_trace_1("StartParam: call Supervisor::Startparam::applyDiff() with empty groups list");
        return 1;
    }

    std::pair<iterator, iterator> range( curConfig_.equal_range(*curConfig_.begin())); //Диапазон групп с самым маленьким приоритетом
    int retValue = 0;
    int sleepSecond = readIntFromTcl("OBRZAP_START_PAUSE", 1);

    while( range.second != curConfig_.end() ) { //До последней группы
        retValue |= applyDiffStep(start, stop, range.first, range.second);
        range = curConfig_.equal_range(*(range.second)); //Следующий диапазон групп с большим приоритетом
        sleep(sleepSecond);
    }
    retValue |= applyDiffStep(start, stop, range.first, range.second);

    OldConfigIterator tmpIt;
    for(OldConfigIterator oldGrp = oldConfig_.begin(); oldGrp != oldConfig_.end(); ) {
        if (!oldGrp->second) {
        //Остались процессы из старой конфигурации, которых в новой нет
            tmpIt = oldGrp++;
            retValue |= changeOldConfig(tmpIt, stop(tmpIt->first.handlersCount(),
                                                      tmpIt->first.startLine(),
                                                      getTclInterpretator()
                                                  )
                        );
        } else {
            oldGrp->second = false;
            ++oldGrp;
        }
    }

    return retValue;
}

template<typename StartFunc, typename StopFunc>
int StartParam::applyDiffStep(StartFunc start, StopFunc stop, iterator beg, iterator end)
{
    int retValue = 0;
    std::string resultStartLine;
    const char* objv[1];

    for(iterator i = beg; i != end; ++i) { //Для каждого элемента с данным приоритетом
        OldConfigIterator oldGrp = oldConfig_.find(*i);
        if (oldConfig_.end() != oldGrp) {
        //Нашли в старой конфигураци группу процессов, соответствующую новой конфигурации
            int diffHandlersCount = i->handlersCount() - oldGrp->first.handlersCount();

            if (diffHandlersCount < 0) {
            //Нужно остановить |diffHandlersCount| процессов
                retValue |= changeOldConfig(oldGrp,
                                            diffHandlersCount,
                                            stop(std::abs(diffHandlersCount),
                                                 oldGrp->first.startLine(),
                                                 getTclInterpretator()
                                                )
                );
            } else if (diffHandlersCount > 0) {
            //Нужно запустить diffHandlersCount процессов
                for(int handlers = 0; handlers < diffHandlersCount; ++handlers) {
                    *objv = i->startLine().c_str();
                    retValue |= changeOldConfig(oldGrp,
                                                +1,
                                                start(1, objv, getTclInterpretator(), i->level()));
                }
            }
            oldGrp->second = true;
        } else {
        // Таких процессов вообще не было в старом конфиге
            for(int handlers = 0; handlers < i->handlersCount(); ++handlers) {
                *objv = i->startLine().c_str();
                retValue |= changeOldConfig(*i, start(1, objv, getTclInterpretator(), i->level()));
            }
        }
    }

    return retValue;
}

} //namespace Supervisor

#endif //_SERVERLIB_STARTPARAM_H_
