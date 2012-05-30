#ifndef _HTTP_IO_
#define _HTTP_IO_

#include<string>
#include<vector>

void http_send_zaglushka(std::vector<std::string> &bsm_bodies);
std::string send_bsm(const std::string host, const std::string &bsm_list);
void my_test();

#endif
