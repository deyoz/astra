#ifndef __BASE64_H
#define __BASE64_H

#include <string>
#include <vector>

size_t base64_get_encoded_size(size_t data_size);
void base64_encode(const unsigned char* data, size_t data_size, std::string& encoded);
void base64_encode(const unsigned char* data, size_t data_size, char* encoded_p, size_t encoded_z);
void base64_decode(const char* encoded, size_t encoded_size, std::vector<unsigned char>& data);
// allows spaces and cr/lf, returns false on decoding error
bool base64_decode_formatted(const char* encoded, size_t encoded_size, std::vector<unsigned char>& data);

#endif /* #ifndef __BASE64_H */
