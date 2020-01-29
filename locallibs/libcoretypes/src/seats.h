#ifndef CORETYPES_SEATS_H
#define CORETYPES_SEATS_H

#include <string>
#include <vector>
#include <set>
#include <iosfwd>
#include <boost/optional.hpp>
#include <serverlib/enumset.h>
#include <serverlib/rip.h>

#define MAXSEATS    15    /* максимальное количество кресел в ряду */
#define MAXSEATSOST  4    /* максимальное количество состояний кресла */
#define MAXSEATSLIST 400  /* максимальное количество мест в списке мест */

namespace ct
{

DECL_RIP(SpaceId, int);

class Seat
{
public:
    static boost::optional<Seat> fromStr(const std::string&);

    unsigned row() const { return row_; } // Номер ряда, нумерация с 1
    unsigned col() const { return col_; } // Номер кресла в ряду, нумерация с 1

    std::string toString() const;

    bool operator==(const Seat& rhp) const;
    bool operator!=(const Seat& rhp) const;
    bool operator<(const Seat& rhp) const;

private:
    Seat(unsigned row, unsigned col);

private:
    unsigned int row_;
    unsigned int col_;
};

int getSeatNum(unsigned char c);
std::string getSeatLetter(unsigned int col);

typedef std::set<Seat> Seats;

std::string seatsToString(const Seats&);
Seats seatsFromString(const std::string&);

std::ostream& operator<<(std::ostream& os, const Seat& seat);

Seats operator+(const Seats& rv, const Seats& lv);
Seats operator-(const Seats& rv, const Seats& lv);
Seats operator&(const Seats& rv, const Seats& lv);

/* AIRIMP37 3.19.2.4 Location Codes */
enum class SeatRequestAttr {
    SMOKING,
    NON_SMOKING,
    WINDOW,
    AISLE,
    BULKHEAD,
    CHARGEABLE,
    EXIT_ROW,
    NON_CHARGEABLE,
    HANDICAPPED,
    WITH_INFANT,
    LEG_SPACE,
    MEDICALLY_OKAY,
    REAR_FACING,
    UNACCOMPANIED_MINOR,
    EXTRA_SEAT_COMFORT
};
std::ostream& operator<<(std::ostream& out, SeatRequestAttr);

typedef EnumSet<SeatRequestAttr> SeatRequestAttrs;
std::ostream& operator<<(std::ostream& out, const SeatRequestAttrs& attrs);

} // ct

#endif /* CORETYPES_SEATS_H */
