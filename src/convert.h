#ifndef _CONVERT_H_
#define _CONVERT_H_

#include <string>

bool is_iata_line(std::string line);
bool is_iata_row(std::string row);
std::string norm_iata_line(std::string line, bool pr_lat = true);
std::string norm_iata_row(std::string row);
std::string denorm_iata_row(std::string row);

#endif
