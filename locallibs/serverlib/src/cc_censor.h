#ifndef _CC_CENSOR_H_
#define _CC_CENSOR_H_

#include <string>
#include <vector>
#include <functional>

bool censure(char *buff) __attribute__((warn_unused_result));

std::string censure(const std::string &buff);

typedef std::pair<size_t, size_t> string_range;

std::vector<string_range> extractCardNumbers(const std::string&, bool strictCheck);

bool containsCardNumbers(const std::string&, bool strictCheck = true);

bool maskString(std::string& s, bool strictCheck, char rpl,
        std::function<void (const std::string&, const std::string&)> trace);

std::string maskCardNumber(const std::string& s, char maskChar);


#endif /*_CC_CENSOR_H_*/
