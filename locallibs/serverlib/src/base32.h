#ifndef BASE32_H
#define BASE32_H

#include <string>
#include <vector>

std::vector<uint8_t> base32_encode(const std::vector<uint8_t>& plain);
std::string base32_encode(const std::string& plain);

std::vector<uint8_t> base32_decode(const std::vector<uint8_t>& encoded);
std::string base32_decode(const std::string& encoded);

#endif /* BASE32_H */
