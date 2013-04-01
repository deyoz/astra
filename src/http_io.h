#ifndef _HTTP_IO_
#define _HTTP_IO_

#include<string>
#include<vector>

std::string send_bsm(const std::string host, const std::string &bsm);

struct HTTPRequestInfo
{
  std::string addr;
  int port;
  std::string resource;
  std::string action;
  std::string content;
};

void sirena_rozysk_send(const HTTPRequestInfo &request);

#endif
