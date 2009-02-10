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
  char *p;
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
  char *p;
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
