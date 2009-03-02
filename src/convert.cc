#include "convert.h"
#include "astra_consts.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include <sstream>
#include <iomanip>
#include "exceptions.h"

using namespace std;
using namespace ASTRA;
using namespace EXCEPTIONS;

// коммент

bool is_iata_line(std::string line)
{
    bool result = false;
    if(line.size() == 1)
        for(size_t i = 0; i < strlen(rus_seat); i++)
            if(line[0] == rus_seat[i] || line[0] == lat_seat[i]) {
                result = true;
                break;
            }
    return result;
}

bool is_iata_row(std::string row)
{
    bool result = false;
    try {
        int row_num = ToInt(row);
        if(row_num >= FIRST_IATA_ROW and row_num <=99 or row_num >= 101 and row_num <= LAST_IATA_ROW)
            result = true;
    } catch(...) {
    }
    return result;
}

string norm_iata_line(std::string line)
{
    return denorm_iata_line(line, true);
}

string denorm_iata_line(std::string line, bool pr_lat)
{
    if(is_iata_line(line)) {
        for(size_t i = 0; i < strlen(rus_seat); i++)
            if(pr_lat and line[0] == rus_seat[i] or not pr_lat and line[0] == lat_seat[i]) {
                pr_lat ? line = lat_seat[i] : line = rus_seat[i];
                break;
            }
    }
    return line;
}

string norm_iata_row(string row)
{
    if(is_iata_row(row)) {
        row = trim(row);
        if(row.size() < 3) {
            ostringstream buf;
            buf << setw(3) << setfill('0') << row;
            row = buf.str();
        } else if(row.size() > 3)
            row = row.substr(row.size() - 3);
    }
    return row;
}

string denorm_iata_row(string row)
{
    if(is_iata_row(row)) {
        row = trim(row);
        size_t i = 0;
        for(; i < row.size(); i++)
            if(row[i] != '0')
                break;
        row = row.substr(i);
    }
    return row;
}

string prev_iata_row(string row)
{
    string result;
    int tmp = NoExists;
    try {
        tmp = ToInt(row);
    } catch(...) {
        throw Exception("prev_iata_row: couldn't convert row '%s'", row.c_str());
    }
    tmp--;
    if(tmp < FIRST_IATA_ROW or tmp > LAST_IATA_ROW)
        throw Exception("prev_iata_row: preceeding row is not in IATA format");
    else {
        // пропускаем разрывы в диапазоне номеров
        // (на момент написания это только цифра 100)
        while(true) {
            result = IntToString(tmp);
            if(is_iata_row(result))
                break;
            tmp--;
        }
    }
    return norm_iata_row(result);
}

string prev_iata_line(string line)
{
    const char *seat = is_lat(line) ? lat_seat : rus_seat;
    size_t i = 0;
    for(; i < strlen(seat); i++)
        if(seat[i] == line[0])
            break;
    string result;
    if(i > 0 and i < strlen(seat))
        result = seat[i - 1];
    return result;
}

string next_iata_line(string line)
{
    const char *seat = is_lat(line) ? lat_seat : rus_seat;
    size_t i = 0;
    for(; i < strlen(seat); i++)
        if(seat[i] == line[0])
            break;
    string result;
    if(i >= 0 and i < strlen(seat) - 1)
        result = seat[i + 1];
    return result;
}

// функция line1 < line2. Можно было и напрямую сравнить, но я че-то побоялся.
bool less_iata_line(std::string line1, std::string line2)
{
    const char *seat = is_lat(line1) ? lat_seat : rus_seat;
    if(is_lat(line2) and seat != lat_seat)
        throw Exception("less_iata_line: unmatched encoding");
    size_t i1 = 0;
    size_t i2 = 0;
    for(; i1 < strlen(seat); i1++)
        if(seat[i1] == line1[0]) break;
    for(; i2 < strlen(seat); i2++)
        if(seat[i2] == line2[0]) break;
    return i1 < i2;
}
