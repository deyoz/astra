#ifndef _PAX_EVENTS_H_
#define _PAX_EVENTS_H_

#include <list>
#include <string>
#include "astra_consts.h"
#include "date_time.h"

class TPaxEventTypes {
    public:
        enum Enum {
            BRD,
            UNBRD,
            Unknown
        };

        static const std::list< std::pair<Enum, std::string> >& pairsCodes()
        {
            static std::list< std::pair<Enum, std::string> > l;
            if (l.empty())
            {
                l.push_back(std::make_pair(BRD,     "BRD"));
                l.push_back(std::make_pair(UNBRD,   "UNBRD"));
                l.push_back(std::make_pair(Unknown, ""));
            }
            return l;
        }
};

class TPaxEventTypesCode: public ASTRA::PairList<TPaxEventTypes::Enum, std::string>
{
  private:
    virtual std::string className() const { return "TPaxEventTypesCode"; }
  public:
    TPaxEventTypesCode() : ASTRA::PairList<TPaxEventTypes::Enum, std::string>(TPaxEventTypes::pairsCodes(),
                                                                            boost::none,
                                                                            boost::none) {}
};

struct TPaxEvent {
    int pax_id;
    TPaxEventTypes::Enum pax_event;
    BASIC::date_time::TDateTime time;
    std::string desk;
    std::string station;
    void toDB(int pax_id, TPaxEventTypes::Enum pax_event);
    bool fromDB(int pax_id, TPaxEventTypes::Enum pax_event);
    TPaxEvent() {
        clear();
    }
    void clear() {
        pax_id = ASTRA::NoExists;
        pax_event = TPaxEventTypes::Unknown;
        time = ASTRA::NoExists;
        desk.clear();
        station.clear();
    }
};

#endif
