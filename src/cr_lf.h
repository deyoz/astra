#ifndef _CR_LF_H_
#define _CR_LF_H_

#include <string>

const std::string CR = "\xd";
const std::string LF = "\xa";
const std::string TAB = "\x9";

void clean_ending(std::string &data, const std::string &end);
std::string place_CR_LF(std::string data);

#endif
