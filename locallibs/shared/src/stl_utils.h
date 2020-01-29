#ifndef stl_utilsH
#define stl_utilsH

#include <string>
#include <vector>
#include <sstream>
#include <map>

std::string& RTrimString(std::string& value);
std::string& LTrimString(std::string& value);
std::string& TrimString(std::string& value);
std::string& NormalizeString(std::string& str);
void SeparateString(const char* str, int len, std::vector<std::string>& strs);
void SeparateString(std::string str, char separator, std::vector<std::string>& strs);
void StringToHex(const std::string& src, std::string& dest);
bool HexToString(const std::string& src, std::string& dest);
std::string upperc(const std::string &tabname);
std::string lowerc(const std::string &tabname);
bool IsAscii7(const std::string &value);
std::string IntToString(int val);
std::string FloatToString( double val, int precision=-1 );
int ToInt(const std::string val);
std::string ConvertCodepage(const std::string& str,
                            const std::string& fromCp,
                            const std::string& toCp);

template<typename T>
std::string tostring(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string::size_type EditDistance(const std::string &s1, const std::string &s2);
int EditDistanceSimilarity(const std::string &s1, const std::string &s2);




#endif


