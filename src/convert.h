#ifndef _CONVERT_H_
#define _CONVERT_H_

#include <string>

bool is_iata_line(std::string line);
bool is_iata_row(std::string row);
// convert iata line to lat
std::string norm_iata_line(std::string line);
// convert iata line depends on pr_lat
std::string denorm_iata_line(std::string line, bool pr_lat);
std::string norm_iata_row(std::string row);
std::string denorm_iata_row(std::string row);
std::string prev_iata_line(std::string line);

#endif
