#ifndef __ORACLE_CONNECTION_PARAM_H_
#define __ORACLE_CONNECTION_PARAM_H_

#include <string>

struct oracle_connection_param
{
    std::string login;
    std::string password;
    std::string server;
};
void split_connect_string(const std::string& connect_string, oracle_connection_param& result);
std::string make_connect_string(const oracle_connection_param& param);
const char* get_connect_string();

#endif // __ORACLE_CONNECTION_PARAM_H_
