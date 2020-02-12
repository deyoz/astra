#include <stdlib.h>
#include "seats_utils.h"
#include "exceptions.h"
#include "stl_utils.h"
#include "astra_consts.h"
#include "misc.h"
#include "seat_number.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace ASTRA;

TSeat::TSeat(const std::string &row_, const std::string &line_)
{
  if (row_.size()>=sizeof(row))
    throw Exception("%s: wrong row '%s'", __func__, row_.c_str());
  if (line_.size()>=sizeof(line))
    throw Exception("%s: wrong line '%s'", __func__, line_.c_str());

  strcpy(row, row_.c_str());
  strcpy(line,line_.c_str());
}

std::string TSeat::toString() const
{
  return string(row) + string(line);
}

std::string TSeat::denorm_view(bool is_lat) const
{
  return SeatNumber::tryDenormalizeRow( row ) + SeatNumber::tryDenormalizeLine( line, is_lat );
}

bool TSeatRanges::contains(const TSeat &seat) const
{
  for(TSeatRanges::const_iterator r=begin(); r!=end(); ++r)
    if (SeatInRange(*r, seat)) return true;
  return false;
}

std::string TSeatRange::traceStr() const
{
  ostringstream s;
  s << first.denorm_view(true);
  if (first!=second)
    s << "-" << second.denorm_view(true);
  return s.str();
}

std::string TSeatRanges::traceStr() const
{
  ostringstream s;
  for(TSeatRanges::const_iterator r=begin(); r!=end(); ++r)
  {
    if (r!=begin()) s << ", ";
    s << r->traceStr();
  }
  return s.str();
}

void NormalizeSeat(TSeat &seat)
{
  boost::optional<string> row = SeatNumber::normalizeIataRow(seat.row);
  if (!row) throw EConvertError("%s: non-IATA row '%s'", __func__, seat.row);
  boost::optional<char> line = SeatNumber::normalizeIataLine(seat.line);
  if (!line) throw EConvertError("%s: non-IATA line '%s'", __func__, seat.line);

  if (row.get().empty() || row.get().size()>=sizeof(seat.row))
    throw EConvertError("%s: error in procedure SeatNumber::normalizeIataRow", __func__);

  seat.line[0]=line.get();
  seat.line[1]=0;
  strcpy(seat.row, row.get().c_str());
}

void NormalizeSeatRange(TSeatRange &range)
{
  NormalizeSeat(range.first);
  NormalizeSeat(range.second);
};

bool NextNormSeatRow(TSeat &seat)
{
  boost::optional<string> row=SeatNumber::normalizeNextIataRow(seat.row);
  if (!row)
  {
    if (!SeatNumber::isIataRow(seat.row))
      throw EConvertError("%s: non-IATA row '%s'", __func__, seat.row);
    return false;
  }

  if (row.get().empty() || row.get().size()>=sizeof(seat.row))
    throw EConvertError("%s: error in procedure SeatNumber::normalizeNextIataRow", __func__);

  strcpy(seat.row, row.get().c_str());
  return true;
}

bool PriorNormSeatRow(TSeat &seat)
{
  boost::optional<string> row=SeatNumber::normalizePrevIataRow(seat.row);
  if (!row)
  {
    if (!SeatNumber::isIataRow(seat.row))
      throw EConvertError("%s: non-IATA row '%s'", __func__, seat.row);
    return false;
  }

  if (row.get().empty() || row.get().size()>=sizeof(seat.row))
    throw EConvertError("%s: error in procedure SeatNumber::normalizePrevIataRow", __func__);

  strcpy(seat.row, row.get().c_str());
  return true;
}

TSeat& FirstNormSeatRow(TSeat &seat)
{
  const boost::optional<string>& row=SeatNumber::normalizeFirstIataRow();
  if (!row || row.get().empty() || row.get().size()>=sizeof(seat.row))
    throw EConvertError("%s: error in procedure SeatNumber::normalizeFirstIataRow", __func__);
  strcpy(seat.row, row.get().c_str());
  return seat;
}

TSeat& LastNormSeatRow(TSeat &seat)
{
  const boost::optional<string>& row=SeatNumber::normalizeLastIataRow();
  if (!row || row.get().empty() || row.get().size()>=sizeof(seat.row))
    throw EConvertError("%s: error in procedure SeatNumber::normalizeFirstIataRow", __func__);
  strcpy(seat.row, row.get().c_str());
  return seat;
}

bool NextNormSeatLine(TSeat &seat)
{
  boost::optional<char> line=SeatNumber::normalizeNextIataLine(seat.line);
  if (!line)
  {
    if (!SeatNumber::isIataLine(seat.line))
      throw EConvertError("%s: non-IATA line '%s'", __func__, seat.line);
    return false;
  }

  seat.line[0]=line.get();
  seat.line[1]=0;
  return true;
}

bool PriorNormSeatLine(TSeat &seat)
{
  boost::optional<char> line=SeatNumber::normalizePrevIataLine(seat.line);
  if (!line)
  {
    if (!SeatNumber::isIataLine(seat.line))
      throw EConvertError("%s: non-IATA line '%s'", __func__, seat.line);
    return false;
  }

  seat.line[0]=line.get();
  seat.line[1]=0;
  return true;
}

TSeat& FirstNormSeatLine(TSeat &seat)
{
  const boost::optional<char>& line=SeatNumber::normalizeFirstIataLine();
  if (!line)
    throw EConvertError("%s: error in procedure SeatNumber::normalizeFirstIataLine", __func__);
  seat.line[0]=line.get();
  seat.line[1]=0;
  return seat;
}

TSeat& LastNormSeatLine(TSeat &seat)
{
  const boost::optional<char>& line=SeatNumber::normalizeLastIataLine();
  if (!line)
    throw EConvertError("%s: error in procedure SeatNumber::normalizeLastIataLine", __func__);
  seat.line[0]=line.get();
  seat.line[1]=0;
  return seat;
}

bool NextNormSeat(TSeat &seat)
{
  if (!NextNormSeatLine(seat))
  {
    if (!NextNormSeatRow(seat)) return false;
    FirstNormSeatLine(seat);
  };
  return true;
};

bool SeatInRange(const TSeatRange &range, const TSeat &seat)
{
  return strcmp(seat.row,range.first.row)>=0 &&
         strcmp(seat.row,range.second.row)<=0 &&
         strcmp(seat.line,range.first.line)>=0 &&
         strcmp(seat.line,range.second.line)<=0;
};

bool NextSeatInRange(const TSeatRange &range, TSeat &seat)
{
  if (!SeatInRange(range,seat)) return false;
  if (strcmp(seat.line,range.second.line)>=0)
  {
    if (strcmp(seat.row,range.second.row)>=0) return false;
    strcpy(seat.line,range.first.line);
    NextNormSeatRow(seat);
  }
  else NextNormSeatLine(seat);
  return true;
};

string GetSeatRangeView(const TSeatRanges &ranges, const string &format, bool pr_lat, int &seats)
{
  //создаем сортированный массив TSeat
  vector<TSeat> iata_seats;
  TSeatRanges not_iata_ranges;
  for(TSeatRanges::const_iterator r=ranges.begin(); r!=ranges.end(); r++)
  {
    TSeatRange iata_range(r->first, r->second);
    try
    {
      NormalizeSeatRange(iata_range);
    }
    catch(EConvertError &)
    {
      if (find(not_iata_ranges.begin(),not_iata_ranges.end(),*r)==not_iata_ranges.end())
        not_iata_ranges.push_back(*r);
      continue;
    };

    TSeat iata_seat=iata_range.first;
    do
    {
      if (find(iata_seats.begin(),iata_seats.end(),iata_seat)==iata_seats.end())
        iata_seats.push_back(iata_seat);
    }
    while (NextSeatInRange(iata_range,iata_seat));
  };

  sort(iata_seats.begin(),iata_seats.end());
  sort(not_iata_ranges.begin(),not_iata_ranges.end());

  string fmt=format;
  boost::optional<char> add_ch;
    if ( fmt == "_list" || fmt == "_one" || fmt == "_seats" )
    {
        add_ch = ' ';
        fmt.erase(0,1);
     };
     ostringstream res;
  for(vector<TSeat>::const_iterator s=iata_seats.begin(); s!=iata_seats.end(); s++)
  {
    if (!res.str().empty())
    {
      if (fmt=="list") res << " "; else break;
    };

    res << SeatNumber::tryDenormalizeRow( s->row, add_ch )
        << SeatNumber::tryDenormalizeLine( s->line, pr_lat );
    add_ch = boost::none;
  };
  for(TSeatRanges::const_iterator r=not_iata_ranges.begin(); r!=not_iata_ranges.end(); r++)
  {
    if (!res.str().empty())
    {
      if (fmt=="list") res << " "; else break;
    };

    res << r->first.row << r->first.line;
    if (fmt=="list" && r->first!=r->second)
      res << "-" << r->second.row << r->second.line;
  };

  seats=iata_seats.size()+not_iata_ranges.size();
  if ( fmt != "list" && fmt != "one" && seats > 1 )
    res << "+" << seats-1;
  return res.str();
};

string GetSeatRangeView(const TSeatRanges &ranges, const string &format, bool pr_lat)
{
  int seats=NoExists;
  return GetSeatRangeView(ranges, format, pr_lat, seats);
};

string GetSeatView(const TSeat &seat, const string &format, bool pr_lat)
{
  return GetSeatRangeView(TSeatRanges(seat), format, pr_lat);
};

