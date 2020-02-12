#pragma once
#include <string>
#include <boost/optional.hpp>

class SeatNumber
{
  private:
    static const std::string rusLines;
    static const std::string latLines;

    static const int firstIataRow=1;
    static const int lastIataRow=199;
    static const int notIataRow=100;

    static boost::optional<char> getIataLineChar(const std::string& line,
                                                 const int shift,
                                                 const boost::optional<bool> toLatin=boost::none);
    static boost::optional<int> getIataRowNum(const std::string& row, const int shift);
    static boost::optional<std::string> denormalizeIataRow(const std::string& row,
                                                           const int shift,
                                                           const boost::optional<char> add_ch);

  public:
    static bool isIataLine(const std::string& line);
    static bool isIataRow(const std::string& row);

    static boost::optional<char> normalizeIataLine(const std::string& line);
    static std::string tryDenormalizeLine(const std::string& line, const bool toLatin);
    static std::string tryNormalizeLine(const std::string& line);
    static boost::optional<char> normalizePrevIataLine(const std::string& line);
    static boost::optional<char> normalizeNextIataLine(const std::string& line);
    static const boost::optional<char>& normalizeFirstIataLine();
    static const boost::optional<char>& normalizeLastIataLine();
    static std::string prevIataLineOrEmptiness(const std::string& line);
    static std::string nextIataLineOrEmptiness(const std::string& line);
    static bool lessIataLine(const std::string& line1, const std::string& line2);

    static boost::optional<std::string> normalizeIataRow(const std::string& row);
    static std::string tryDenormalizeRow(const std::string& row, const boost::optional<char> add_ch=boost::none);
    static std::string tryNormalizeRow(const std::string& row);
    static boost::optional<std::string> normalizePrevIataRow(const std::string& row);
    static boost::optional<std::string> normalizeNextIataRow(const std::string& row);
    static const boost::optional<std::string>& normalizeFirstIataRow();
    static const boost::optional<std::string>& normalizeLastIataRow();
    static std::string normalizePrevIataRowOrException(const std::string& row);
};



