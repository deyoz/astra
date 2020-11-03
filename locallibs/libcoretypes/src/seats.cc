#ifdef HAVE_MPATROL
#include <mpatrol.h>
#endif

#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>

#include <serverlib/helpcpp.h>
#include <serverlib/string_cast.h>

#include "seats.h"

#define NICKNAME "NONSTOP"
#include <serverlib/slogger.h>

// Коды кресел в ряду
static const unsigned char SEATSIMBOL[ MAXSEATS + 1] = { "ABCDEFGHIJKLMNO" };

class SeatsParser
{
    enum State {WaitNums, WaitChars};
    typedef std::string::size_type pos_t;
public:
    SeatsParser(ct::Seats& seats)
        : seats_(seats), state_(WaitNums)
    {
        clearTokens();
    }
    bool parse(const std::string& str) {
        const size_t len(str.length());
        size_t pos = 0;
        while (pos < len) {
            const char c(str[pos]);
            if (!isDelim(c) && !isNum(c) && !isAlpha(c)) {
                LogTrace(TRACE5) << str << " bad char: [" << c << "] at " << pos;
                return false;
            }
            ++pos;
        }
        pos = 0;
        while (pos < len) {
            const char c(str[pos]);
            if (!isNum(c)) {
                LogTrace(TRACE5) << str << " bad char: [" << c << "] at " << pos << " expected num";
                return false;
            }
            numsToken.first = pos;
            pos_t posEnd = str.find_first_not_of("1234567890-", pos + 1);
            if (posEnd == std::string::npos) {
                LogTrace(TRACE5) << str << " bad char: [" << c << "] at " << pos << " expected char";
                return false;
            }
            numsToken.second = posEnd - 1;

            pos = numsToken.second + 1;
            charsToken.first = pos;
            posEnd = str.find_first_of("1234567890", pos + 1);
            charsToken.second = (posEnd == std::string::npos) ? len - 1 : posEnd - 1;
            pos = charsToken.second + 1;

            // process tokens
            int n1(0), n2(0);
            if (!parseNumsTokens(str, n1, n2)) {
                return false;
            }
            if (n1 > n2) {
                LogTrace(TRACE5) << str << " invalid nums token values: " << n1 << '-' << n2;
                return false;
            }

            char c1(0), c2(0);
            if (!parseCharsTokens(str, c1, c2)) {
                return false;
            }
            if (c1 > c2) {
                LogTrace(TRACE5) << str << " invalid chars token values: " << c1 << '-' << c2;
                return false;
            }

            for (int i = n1; i <= n2; ++i) {
                const std::string ns(HelpCpp::string_cast(i));
                for (char c = c1; c <= c2; ++c) {
                    const std::string seatStr(ns + std::string(1, c));
                    boost::optional<ct::Seat> s(ct::Seat::fromStr(seatStr));
                    if (!s) {
                        LogError(STDLOG) << "failed Seat::fromStr [" << seatStr << "]";
                        return false;
                    }
                    seats_.insert(*s);
                }
            }
        }
        return true;
    }
    bool parseNumsTokens(const std::string& str, int& n1, int& n2) const {
        pos_t delimPos = findDelim(str, numsToken);
        if (delimPos == std::string::npos) {
            n1 = n2 = atoi(str.substr(numsToken.first, numsToken.second - numsToken.first + 1).c_str());
            return true;
        }
        if (pos_t p = findDelim(str, std::make_pair(delimPos + 1, numsToken.second)) != std::string::npos) {
            LogTrace(TRACE5) << str << " bad char: [" << str[p] << "] at " << p << " unexpected -";
            return false;
        }
        n1 = atoi(str.substr(numsToken.first, delimPos - numsToken.first).c_str());
        n2 = atoi(str.substr(delimPos + 1, numsToken.second - delimPos).c_str());
        return true;
    }

    pos_t findDelim(const std::string& str, const std::pair<pos_t, pos_t>& token) const {
        for (pos_t i = token.first; i <= token.second; ++i) {
            if (str[i] == '-') {
                return i;
            }
        }
        return std::string::npos;
    }

    bool parseCharsTokens(const std::string& str, char& c1, char& c2) const {
        const size_t len = charsToken.second - charsToken.first;
        switch (len) {
        case 0:
            c1 = c2 = str[charsToken.first];
            return c1 != '-' && c2 != '-';
        case 1:
            c1 = str[charsToken.first];
            c2 = str[charsToken.second];
            return c1 != '-' && c2 != '-';
        case 2:
            c1 = str[charsToken.first];
            c2 = str[charsToken.second];
            return str[charsToken.first + 1] == '-' && c1 != '-' && c2 != '-';
        default:
            LogTrace(TRACE5) << str << " invalid chars token length " << len;
            return false;
        }
    }
    void clearTokens() {
        numsToken.first = numsToken.second = -1;
        charsToken.first = charsToken.second = -1;
    }
private:
    bool isDelim(char c) const {
        return c == '-';
    }
    bool isNum(char c) const {
        return 0 <= (c - '0') && (c - '0') <= 9;
    }
    bool isAlpha(char c) const {
        return 'A' <= c && c <= 'Z';
    }
    ct::Seats& seats_;
    State state_;
    std::pair<pos_t, pos_t> numsToken, charsToken;
};

namespace ct
{

Seat::Seat(unsigned r, unsigned c)
    : row_(r), col_(c)
{
    ASSERT(!(row_ == 0 || row_ > MAXSEATSLIST || col_ == 0));
}

boost::optional<Seat> Seat::fromStr(const std::string& str)
{
    if (!(str.length() > 1 && str.length() < 5)) {
        LogTrace(TRACE5) << "invalid Seat str: [" << str << ']';
        return boost::optional<Seat>();
    }

    const char letter = *(--str.end());
    const int col = getSeatNum(letter);
    const unsigned int row = boost::lexical_cast<unsigned int>(std::string(str.begin(), --str.end()));
    if (row == 0 || row > MAXSEATSLIST || col < 0) {
        LogTrace(TRACE5) << "invalid seat row and/or col: row = "
            << row << ", col = " << col;
        return boost::optional<Seat>();
    }
    return Seat(row, col + 1);
}

bool Seat::operator==(const Seat& rhp) const
{
    return row_ == rhp.row_
        && col_ == rhp.col_;
}

bool Seat::operator!=(const Seat& rhp) const
{
    return !(*this == rhp);
}

bool Seat::operator<(const Seat& rhs) const
{
    return (this->col_ != rhs.col())
        ? (this->col_ < rhs.col())
        : (this->row_ < rhs.row());
}

std::string Seat::toString() const
{
    return HelpCpp::string_cast(row_) + std::string(1, SEATSIMBOL[col_ - 1]);
}

class SeatLess
{
public:
    bool operator()(const ct::Seat& lhs, const ct::Seat& rhs) {
        return lhs.col() < rhs.col();
    }
};

// По букве-номеру места в ряду получить номер кресла в ряду
int getSeatNum(unsigned char c)
{
    for (int i = 0; i < MAXSEATS; ++i) {
        if (c == SEATSIMBOL[i]) {
            return i;
        }
    }

    return -1;
}

std::string getSeatLetter(unsigned int col)
{
    return std::string(1, SEATSIMBOL[col - 1]);
}

std::string seatsToString(const Seats& seats)
{
    ct::Seats::iterator begin = seats.begin();
    std::string result;
    while (seats.end() != begin) {
        const std::pair<ct::Seats::iterator, ct::Seats::iterator>& range = std::equal_range(begin, seats.end(), *begin, SeatLess());
        unsigned startRow = range.first->row();
        unsigned lastRow = range.first->row();
        for (ct::Seats::iterator i = range.first; ++i != range.second; ) {
            if ((1 + lastRow) != i->row()) {
                result += boost::lexical_cast<std::string>(startRow);
                if (lastRow != startRow) {
                    if (1 < (lastRow - startRow)) {
                        result += '-';
                    } else {
                        result += SEATSIMBOL[ range.first->col() - 1 ];
                    }
                    result += boost::lexical_cast<std::string>(lastRow);
                }

                result += SEATSIMBOL[ range.first->col() - 1 ];
                startRow = i->row();
            }

            lastRow = i->row();
        }
        result += boost::lexical_cast<std::string>(startRow);
        if (lastRow != startRow) {
            if (1 < (lastRow - startRow)) {
                result += '-';
            } else {
                result += SEATSIMBOL[ range.first->col() - 1 ];
            }
            result += boost::lexical_cast<std::string>(lastRow);
        }

        result += SEATSIMBOL[ range.first->col() - 1 ];
        begin = range.second;
    }

    return result;
}

Seats seatsFromString(const std::string& str)
{
    Seats seats;
    SeatsParser sp(seats);
    if (!sp.parse(str)) {
        return Seats();
    }

    return seats;
}

std::ostream& operator<<(std::ostream& os, const Seat& seat)
{
    return os << seat.toString();
}

Seats operator+(const Seats& rv, const Seats& lv)
{
    Seats sl(rv);
    sl.insert(lv.begin(), lv.end());

    return sl;
}

Seats operator-(const Seats& rv, const Seats& lv)
{
    Seats out;
    for(const Seat & s:  rv) {
        if (lv.end() == lv.find(s)) {
            out.insert(s);
        }
    }

    return out;
}

Seats operator&(const Seats& rv, const Seats& lv)
{
    Seats out;
    for(const Seats::value_type & s:  rv) {
        if (lv.end() != lv.find(s)) {
            out.insert(s);
        }
    }

    return out;
}

static std::string SeatRequestAttr2Str(SeatRequestAttr attr)
{
    switch (attr) {
        case SeatRequestAttr::SMOKING: return "SMOKING";
        case SeatRequestAttr::NON_SMOKING: return "NON_SMOKING";
        case SeatRequestAttr::WINDOW: return "WINDOW";
        case SeatRequestAttr::AISLE: return "AISLE";
        case SeatRequestAttr::BULKHEAD: return "BULKHEAD";
        case SeatRequestAttr::CHARGEABLE: return "CHARGEABLE";
        case SeatRequestAttr::EXIT_ROW: return "EXIT_ROW";
        case SeatRequestAttr::NON_CHARGEABLE: return "NON_CHARGEABLE";
        case SeatRequestAttr::HANDICAPPED: return "HANDICAPPED";
        case SeatRequestAttr::WITH_INFANT: return "WITH_INFANT";
        case SeatRequestAttr::LEG_SPACE: return "LEG_SPACE";
        case SeatRequestAttr::MEDICALLY_OKAY: return "MEDICALLY_OKAY";
        case SeatRequestAttr::REAR_FACING: return "REAR_FACING";
        case SeatRequestAttr::UNACCOMPANIED_MINOR: return "UNACCOMPANIED_MINOR";
        case SeatRequestAttr::EXTRA_SEAT_COMFORT: return "EXTRA_SEAT_COMFORT";
    }
    LogError(STDLOG) << "invalid SeatRequestAttr: " << attr;
    throw comtech::Exception(STDLOG, __FUNCTION__, "invalid SeatRequestAttr");
}

std::ostream& operator<<(std::ostream& out, SeatRequestAttr attr)
{
    return out << SeatRequestAttr2Str(attr);
}

std::ostream& operator<<(std::ostream& out, const SeatRequestAttrs& attrs)
{
    out << "[";
    for (SeatRequestAttr attr : attrs)
        out << " " << attr;
    out << " ]";
    return out;
}
} // ct

#ifdef XP_TESTING
void init_seats_tests() {}

#include <serverlib/xp_test_utils.h>
#include <serverlib/checkunit.h>

namespace
{

START_TEST(chk_form2seat)
{
    struct {
        const char* str;
        int res;
    } checks[] = {
        {"01A", 1},
        {"1A", 1},
        {"1A1B", 2},
        {"1B1D", 2},
        {"01A01B", 2},
        {"01AB", 2},
        {"01A-E", 5},
        {"01A-5E", 0},
        {"1A-1E", 0},
        {"01A-05E", 0},
        {"0A", 0},
        {"3Z", 0},
        {"SS", 0},
        {"33", 0},
        {"01A-0E", 0},
        {"1В1Г", 0},
        {"3A3B3C3D3E3F4A4B4C4D4E4F5A5B5C5D5E5F6A6B6C6D6E6F7A7B7C7D7E7F8A8B8C8D8E8F9A9B9C9D9E9F10A10B10C10D10E10F11A11B11C11D11E11F12A12B12C12D12E12F13A13B13C13D13E13F14A14B14C14D14E14F15A15B15C15D15E15F16A16B16C16D16E16F17A17B17C17D17E17F18A18B18C18D18E18F19A19B19C19D", 100},
        {"3-19A3-19B3-19C3-19D3-18E3-18F", 100},
        {"03A-19A", 0},
    };
    using ct::seatsFromString;
    for (const auto& c : checks) {
        const int sz = seatsFromString(c.str).size();
        if (sz != c.res) {
            fail_if(true, "%s failed: expected %d got %d", c.str, c.res, sz);
        }
    }
}
END_TEST

START_TEST(checkSeatsToString)
{
    std::vector<const char*> seatList = {
        "1A", "2A", "3A",
        "99A",
        "1B",
        "3B", "4B", "5B",
        "1C", "2C",
        "1E",
        "7A",
        "3G", "4G", "5G", "6G", "7G",
    };
    ct::Seats seats;
    for (const char* s : seatList) {
        seats.insert(*ct::Seat::fromStr(s));
    }
    fail_unless(ct::seatsToString(seats) == "1-3A7A99A1B3-5B1C2C1E3-7G",
                                  "seatsToString failed: %s != 1-3A7A99A1B3-5B1C2C1E3-7G",
                                  ct::seatsToString(seats).c_str());
} END_TEST

#define SUITENAME "coretypes"
TCASEREGISTER(0, 0)
{
    ADD_TEST(chk_form2seat);
    ADD_TEST(checkSeatsToString);
}
TCASEFINISH

} // namespace

#endif // XP_TESTING
