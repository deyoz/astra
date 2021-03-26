#pragma once

#include <string>
#include <serverlib/rip.h>
#include <serverlib/rip_validators.h>
#include <serverlib/dates.h>
#include <boost/date_time/posix_time/posix_time.hpp>

DECL_RIP_RANGED(RowId_t, int, 1, 999999999);

class HistoryEventId
{
  private:
    int order_;
    Dates::DateTime_t time_;
  public:
    HistoryEventId(const int order, const Dates::DateTime_t& time) :
      order_(order), time_(time) {}
    HistoryEventId();

    int order() const { return order_; }
    Dates::DateTime_t time() const { return time_; }
};

class HistoryTable
{
  private:
    int tableId_;
    std::string tableName_;
    std::string identField_;
    std::string historyFields_;

    bool lockRow(const RowId_t& rowId);
    std::optional<HistoryEventId> addRow(const RowId_t& rowId);
    std::optional<HistoryEventId> getLastEventId(const RowId_t& rowId);
    void closeLastEvent(const RowId_t& rowId,
                        const std::optional<HistoryEventId>& nextEventId);
    void openNextEvent(const RowId_t& rowId,
                       const HistoryEventId& nextEventId);

    static const Dates::DateTime_t& upperBoundTime()
    {
      static Dates::DateTime_t upperBoundTime_=Dates::time_from_string("2099-01-01 00:00:00");
      return upperBoundTime_;
    }
  public:
    HistoryTable(const std::string& tableName);
    void synchronize(const RowId_t& rowId);

};


