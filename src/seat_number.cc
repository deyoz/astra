#include "seat_number.h"
#include <sstream>
#include <iomanip>
#include "misc.h"
#include "stl_utils.h"
#include "exceptions.h"

#define NICKNAME "VLAD"
#define NICKTRACE SYSTEM_TRACE
#include <serverlib/slogger.h>

using namespace std;
using namespace EXCEPTIONS;

const std::string SeatNumber::rusLines="ÄÅÇÉÑÖÜáàäãåçéèêëíìîïñóòô";
const std::string SeatNumber::latLines="ABCDEFGHJKLMNOPQRSTUVWXYZ";

boost::optional<char> SeatNumber::getIataLineChar(const std::string& line,
                                                  const int shift,
                                                  const boost::optional<bool> toLatin)
{
  if (line.size()==1)
  {
    bool linesFromIsLatin=IsAscii7(line[0]);
    bool linesToIsLatin=toLatin?toLatin.get():linesFromIsLatin;

    const string& linesFrom=linesFromIsLatin?latLines:rusLines;
    const string& linesTo=  linesToIsLatin?  latLines:rusLines;

    size_t pos=linesFrom.find_first_of(line[0]);
    if (shift!=0 && pos!=string::npos)
    {
      if (shift>0)
      {
        if (pos<linesTo.size()-1) ++pos; else pos=string::npos;
      }
      else
      {
        if (pos>0) --pos; else pos=string::npos;
      }
    }
    if (pos!=string::npos)
      return linesTo.at(pos);
  }

  return boost::none;
}

boost::optional<int> SeatNumber::getIataRowNum(const std::string& row, const int shift)
{
  try
  {
    int row_num = ToInt(row);
    if (shift!=0 && row_num >= firstIataRow && row_num <= lastIataRow && row_num != notIataRow)
    {
      if (shift>0)
      {
        if (++row_num==notIataRow) ++row_num;
      }
      else
      {
        if (--row_num==notIataRow) --row_num;
      }

    }
    if (row_num >= firstIataRow && row_num <= lastIataRow && row_num != notIataRow) return row_num;

  }
  catch(const EConvertError&) {}

  return boost::none;
}

bool SeatNumber::isIataLine(const std::string& line)
{
  return getIataLineChar(line, 0);
}

bool SeatNumber::isIataRow(const std::string& row)
{
  return getIataRowNum(row, 0);
}

boost::optional<bool> SeatNumber::isLatinIataLine(const std::string& line)
{
  boost::optional<char> result=getIataLineChar(line, 0);
  if (result) return IsAscii7(result.get());
  return boost::none;
}

boost::optional<char> SeatNumber::normalizeIataLine(const std::string& line)
{
  return getIataLineChar(line, 0, true);
}

string SeatNumber::tryDenormalizeLine(const std::string& line, const bool toLatin)
{
  boost::optional<char> result=getIataLineChar(line, 0, toLatin);
  return result?string(1,result.get()):line;
}

string SeatNumber::tryNormalizeLine(const std::string& line)
{
  boost::optional<char> result=getIataLineChar(line, 0, true);
  return result?string(1,result.get()):line;
}

boost::optional<char> SeatNumber::normalizePrevIataLine(const std::string& line)
{
  return getIataLineChar(line, -1, true);
}

boost::optional<char> SeatNumber::normalizeNextIataLine(const std::string& line)
{
  return getIataLineChar(line, 1, true);
}

const boost::optional<char>& SeatNumber::normalizeFirstIataLine()
{
  static const boost::optional<char> result=getIataLineChar(string(1, latLines.front()), 0, true);
  return result;
}

const boost::optional<char>& SeatNumber::normalizeLastIataLine()
{
  static const boost::optional<char> result=getIataLineChar(string(1, latLines.back()), 0, true);
  return result;
}

string SeatNumber::prevIataLineOrEmptiness(const string& line)
{
  boost::optional<char> result=getIataLineChar(line, -1);
  return result?string(1, result.get()):"";
}

string SeatNumber::nextIataLineOrEmptiness(const string& line)
{
  boost::optional<char> result=getIataLineChar(line, 1);
  return result?string(1, result.get()):"";
}

bool SeatNumber::lessIataLine(const string& line1, const string& line2)
{
  static bool rusLinesIsSorted=std::is_sorted(rusLines.begin(), rusLines.end());
  static bool latLinesIsSorted=std::is_sorted(latLines.begin(), latLines.end());
  if (!rusLinesIsSorted || !latLinesIsSorted)
    throw Exception("%s: rusLines or latLines not sorted", __func__);
  if (IsAscii7(line1)!=IsAscii7(line2))
    throw Exception("%s: unmatched encoding (line1=%s, line2=%s)", __func__, line1.c_str(), line2.c_str());
  bool isIataLine1=isIataLine(line1);
  bool isIataLine2=isIataLine(line2);
  if (isIataLine1!=isIataLine2)
    return isIataLine1>isIataLine2;
  return line1<line2;
}

boost::optional<string> SeatNumber::denormalizeIataRow(const std::string& row,
                                                       const int shift,
                                                       const boost::optional<char> add_ch)
{
  boost::optional<int> row_num=getIataRowNum(row, shift);
  if (row_num)
  {
    ostringstream buf;
    if (add_ch)
      buf << setw(3) << setfill(add_ch.get());
    buf << row_num.get();
    return buf.str();
  }

  return boost::none;
}

boost::optional<string> SeatNumber::normalizeIataRow(const std::string& row)
{
  return denormalizeIataRow(row, 0, '0');
}

string SeatNumber::tryDenormalizeRow(const std::string& row, boost::optional<char> add_ch)
{
  boost::optional<string> result=denormalizeIataRow(row, 0, add_ch);
  return result?result.get():row;
}

string SeatNumber::tryNormalizeRow(const string& row)
{
  boost::optional<string> result=denormalizeIataRow(row, 0, '0');
  return result?result.get():row;
}

boost::optional<string> SeatNumber::normalizePrevIataRow(const string& row)
{
  return denormalizeIataRow(row, -1, '0');
}

boost::optional<string> SeatNumber::normalizeNextIataRow(const string& row)
{
  return denormalizeIataRow(row, 1, '0');
}

const boost::optional<string>& SeatNumber::normalizeFirstIataRow()
{
  static const boost::optional<string> result=denormalizeIataRow(IntToString(firstIataRow), 0, '0');
  return result;
}

const boost::optional<string>& SeatNumber::normalizeLastIataRow()
{
  static const boost::optional<string> result=denormalizeIataRow(IntToString(lastIataRow), 0, '0');
  return result;
}

string SeatNumber::normalizePrevIataRowOrException(const string& row)
{
  boost::optional<string> result=denormalizeIataRow(row, -1, '0');
  if (!result)
  {
    if (!denormalizeIataRow(row, 0, '0'))
      throw Exception("%s: couldn't convert row '%s'", __func__, row.c_str());
    else
      throw Exception("%s: preceeding row is not in IATA format", __func__);
  }

  return result.get();
}

string SeatNumber::stupidlyChangeEncoding(const std::string& seatNumber)
{
  string result = seatNumber;
  if ( seatNumber == CharReplace( result, rusLines, latLines ) )
    CharReplace( result, latLines, rusLines );
  if ( seatNumber == result )
    result.clear();

  return result;
}
