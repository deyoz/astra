#ifndef CODE_CONVERT_H
#define CODE_CONVERT_H

#include <string>
using namespace std;

// system types:
// MERIDIAN
// AODBO
// AODBI

string AirlineToInternal(const string& external, const string& system_name);
string AirlineToExternal(const string& internal, const string& system_name);

string AirportToInternal(const string& external, const string& system_name);
string AirportToExternal(const string& internal, const string& system_name);

string CraftToInternal(const string& external, const string& system_name);
string CraftToExternal(const string& internal, const string& system_name);

string LiteraToInternal(const string& external, const string& system_name);
string LiteraToExternal(const string& internal, const string& system_name);

#endif // CODE_CONVERT_H
