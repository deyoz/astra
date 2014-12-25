#ifndef _HTTP_IO_
#define _HTTP_IO_

#include<string>
#include<vector>

std::string send_format_ayt(const std::string &uri, const std::string &bsm);

struct HTTPRequestInfo
{
  std::string resource;
  std::string action;
  std::string content;
  std::string login;
  std::string pswd;
};

void sirena_rozysk_send(const HTTPRequestInfo &request);

#endif
