#include <stdlib.h>
#include "seats_utils.h"
#include "exceptions.h"
#include "convert.h"
#include "stl_utils.h"
#include "astra_consts.h"

#define NICKNAME "DJEK"
#include "serverlib/test.h"

using namespace std;
using namespace EXCEPTIONS;
using namespace BASIC;
using namespace ASTRA;


void NormalizeSeat(TSeat &seat)
{
  if (!is_iata_row(seat.row)) throw EConvertError("NormalizeSeat: non-IATA row '%s'",seat.row);
  if (!is_iata_line(seat.line)) throw EConvertError("NormalizeSeat: non-IATA line '%s'",seat.line);

  strncpy(seat.row,norm_iata_row(seat.row).c_str(),sizeof(seat.row));
  strncpy(seat.line,norm_iata_line(seat.line).c_str(),sizeof(seat.line));

  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("NormalizeSeat: error in procedure norm_iata_row");
  if (strnlen(seat.line,sizeof(seat.line)) >= sizeof(seat.line))
    throw EConvertError("NormalizeSeat: error in procedure norm_iata_line");
};

void NormalizeSeatRange(TSeatRange &range)
{
  NormalizeSeat(range.first);
  NormalizeSeat(range.second);
};

bool NextNormSeatRow(TSeat &seat)
{
  int i;
  if (StrToInt(seat.row,i)==EOF)
    throw EConvertError("NextNormSeatRow: error in procedure norm_iata_row");
  if (i==199) return false;
  if (i==99) i=101; else i++;
  strncpy(seat.row,norm_iata_row(IntToString(i)).c_str(),sizeof(seat.row));
  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("NextNormSeatRow: error in procedure norm_iata_row");
  return true;
};

bool PriorNormSeatRow(TSeat &seat)
{
  int i;
  if (StrToInt(seat.row,i)==EOF)
    throw EConvertError("PriorNormSeatRow: error in procedure norm_iata_row");
  if (i==1) return false;
  if (i==101) i=99; else i--;
  strncpy(seat.row,norm_iata_row(IntToString(i)).c_str(),sizeof(seat.row));
  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("PriorNormSeatRow: error in procedure norm_iata_row");
  return true;
};

TSeat& FirstNormSeatRow(TSeat &seat)
{
  strncpy(seat.row,norm_iata_row(IntToString(1)).c_str(),sizeof(seat.row));
  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("FirstNormSeatRow: error in procedure norm_iata_row");
  return seat;
};

TSeat& LastNormSeatRow(TSeat &seat)
{
  strncpy(seat.row,norm_iata_row(IntToString(199)).c_str(),sizeof(seat.row));
  if (strnlen(seat.row,sizeof(seat.row)) >= sizeof(seat.row))
    throw EConvertError("LastNormSeatRow: error in procedure norm_iata_row");
  return seat;
};


bool NextNormSeatLine(TSeat &seat)
{
  const char *p;
  if ((p=strchr(lat_seat,seat.line[0]))==NULL)
    throw EConvertError("NextNormSeatLine: error in procedure norm_iata_line");
  p++;
  if (*p==0) return false;
  seat.line[0]=*p;
  seat.line[1]=0;
  return true;
};

bool PriorNormSeatLine(TSeat &seat)
{
  const char *p;
  if ((p=strchr(lat_seat,seat.line[0]))==NULL)
    throw EConvertError("PriorNormSeatLine: error in procedure norm_iata_line");
  if (p==lat_seat) return false;
  p--;
  seat.line[0]=*p;
  seat.line[1]=0;
  return true;
};

TSeat& FirstNormSeatLine(TSeat &seat)
{
  if (strlen(lat_seat)<=0)
    throw EConvertError("FirstNormSeatLine: lat_seat is empty!");
  seat.line[0]=lat_seat[0];
  seat.line[1]=0;
  return seat;
};

TSeat& LastNormSeatLine(TSeat &seat)
{
  if (strlen(lat_seat)<=0)
    throw EConvertError("LastNormSeatLine: lat_seat is empty!");
  seat.line[0]=lat_seat[strlen(lat_seat)-1];
  seat.line[1]=0;
  return seat;
};

bool NextNormSeat(TSeat &seat)
{
  if (!NextNormSeatLine(seat))
  {
    if (!NextNormSeatRow(seat)) return false;
    FirstNormSeatLine(seat);
  };
  return true;
};

bool SeatInRange(TSeatRange &range, TSeat &seat)
{
  return strcmp(seat.row,range.first.row)>=0 &&
         strcmp(seat.row,range.second.row)<=0 &&
         strcmp(seat.line,range.first.line)>=0 &&
         strcmp(seat.line,range.second.line)<=0;
};

bool NextSeatInRange(TSeatRange &range, TSeat &seat)
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

string GetSeatRangeView(const vector<TSeatRange> &ranges, const string &format, bool pr_lat, int &seats)
{
  //создаем сортированный массив TSeat
  vector<TSeat> iata_seats;
  vector<TSeatRange> not_iata_ranges;
  for(vector<TSeatRange>::const_iterator r=ranges.begin(); r!=ranges.end(); r++)
  {
    TSeatRange iata_range(r->first, r->second);
    try
    {
      NormalizeSeatRange(iata_range);
    }
    catch(EConvertError)
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
  const char* add_ch = NULL;
 	if ( fmt == "_list" || fmt == "_one" || fmt == "_seats" )
 	{
  		add_ch = " ";
	  	fmt.erase(0,1);
	 };
	 ostringstream res;
  for(vector<TSeat>::const_iterator s=iata_seats.begin(); s!=iata_seats.end(); s++)
  {
    if (!res.str().empty())
    {
      if (fmt=="list") res << " "; else break;
    };
  
    res << denorm_iata_row( s->row, add_ch )
        << denorm_iata_line( s->line, pr_lat );
    add_ch = NULL;
  };
  for(vector<TSeatRange>::const_iterator r=not_iata_ranges.begin(); r!=not_iata_ranges.end(); r++)
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

string GetSeatRangeView(const vector<TSeatRange> &ranges, const string &format, bool pr_lat)
{
  int seats=NoExists;
  return GetSeatRangeView(ranges, format, pr_lat, seats);
};

string GetSeatView(const TSeat &seat, const string &format, bool pr_lat)
{
  return GetSeatRangeView(vector<TSeatRange>(1,TSeatRange(seat,seat)), format, pr_lat);
};
