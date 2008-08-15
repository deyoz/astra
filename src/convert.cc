#include "convert.h"
#include "astra_consts.h"
#include "stl_utils.h"
#include "astra_utils.h"
#include <sstream>
#include <iomanip>

using namespace std;
using namespace ASTRA;

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
        int row_num = StrToInt(row);
        if(row_num >= 1 and row_num <=99 or row_num >= 101 and row_num <= 199)
            result = true;
    } catch(...) {
    }
    return result;
}

string norm_iata_line(std::string line, bool pr_lat)
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
