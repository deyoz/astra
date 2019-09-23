#ifndef _SEATS_UTILS_H_
#define _SEATS_UTILS_H_

#include "astra_consts.h"
#include "astra_utils.h"
#include <map>
#include <libxml/tree.h>

class TSeat
{
  public:
    char row[5]; //001-099,101-199
    char line[5];//A-Z...
    TSeat()
    {
      Clear();
    };
    TSeat(const std::string &row, const std::string &line)
    {
      strncpy(this->row,row.c_str(),sizeof(this->row));
      strncpy(this->line,line.c_str(),sizeof(this->line));
    };
    void Clear()
    {
      *row=0;
      *line=0;
    };
    bool Empty() const
    {
      return *row==0 || *line==0;
    };

    TSeat& operator = ( const TSeat& seat )
    {
      if (this == &seat) return *this;
      strncpy(this->row,seat.row,sizeof(this->row));
      strncpy(this->line,seat.line,sizeof(this->line));
      return *this;
    };

    bool operator == ( const TSeat& seat ) const
    {
      return strcmp(row,seat.row)==0 &&
             strcmp(line,seat.line)==0;
    };

    bool operator != ( const TSeat& seat ) const
    {
      return !(strcmp(row,seat.row)==0 &&
               strcmp(line,seat.line)==0);
    };

    bool operator < ( const TSeat& seat ) const
    {
      int res;
      res=strcmp(row,seat.row);
      if (res==0)
        res=strcmp(line,seat.line);
      return res<0;
    };

    std::string denorm_view(bool is_lat) const;
};

class TSeatRange : public std::pair<TSeat,TSeat>
{
  public:
    char rem[5];
    TSeatRange() : std::pair<TSeat,TSeat>()
    {
      *rem=0;
    }
    TSeatRange(const TSeat& seat1, const TSeat& seat2) : std::pair<TSeat,TSeat>(seat1,seat2)
    {
      *rem=0;
    }
    TSeatRange(const TSeat& seat) : std::pair<TSeat,TSeat>(seat,seat)
    {
      *rem=0;
    }
    TSeatRange(const TSeat& seat1, const TSeat& seat2, const std::string &rem) : std::pair<TSeat,TSeat>(seat1,seat2)
    {
      strncpy(this->rem,rem.c_str(),sizeof(this->rem));
    }
    bool operator < ( const TSeatRange& range ) const
    {
      if (first!=range.first)
        return first<range.first;
      return second<range.second;
    }
    std::string traceStr() const;
};

class TSeatRanges : public std::vector<TSeatRange>
{
  public:
    TSeatRanges(const TSeat &seat) : std::vector<TSeatRange>(1,TSeatRange(seat,seat)) {}
    TSeatRanges() : std::vector<TSeatRange>() {}
    bool contains(const TSeat &seat) const;
    std::string traceStr() const;
};

struct CompareSeat  {
  bool operator() ( const TSeat &seat1, const TSeat &seat2 ) const {
    if ( seat1 != seat2 ) {
      return ( seat1 < seat2 );
    }
    return false;
  }
};

class TPassSeats: public std::set<TSeat,CompareSeat> {
  public:
    bool operator == (const TPassSeats &seats) const {
      if ( size() != seats.size() ) {
        return false;
      }
      for ( std::set<TSeat>::const_iterator iseat1=begin(),
            iseat2=seats.begin();
            iseat1!=end(), iseat2!=seats.end(); iseat1++, iseat2++ ) {
        if (  *iseat1 != *iseat2 ) {
          return false;
        }
      }
      return true;
    }
};


//функция кроме представления номера места возвращает кол-во мест
std::string GetSeatRangeView(const TSeatRanges &ranges,
                             const std::string &format,
                             bool pr_lat,
                             int &seats);

std::string GetSeatRangeView(const TSeatRanges &ranges,
                             const std::string &format,
                             bool pr_lat);

std::string GetSeatView(const TSeat &seat,
                        const std::string &format,
                        bool pr_lat);

//все нижеследующие функции работают только с IATA (нормальными) местами!
void NormalizeSeat(TSeat &seat);
void NormalizeSeatRange(TSeatRange &range);
bool NextNormSeatRow(TSeat &seat);
bool PriorNormSeatRow(TSeat &seat);
TSeat& FirstNormSeatRow(TSeat &seat);
TSeat& LastNormSeatRow(TSeat &seat);
bool NextNormSeatLine(TSeat &seat);
bool PriorNormSeatLine(TSeat &seat);
TSeat& FirstNormSeatLine(TSeat &seat);
TSeat& LastNormSeatLine(TSeat &seat);
bool NextNormSeat(TSeat &seat);
bool SeatInRange(const TSeatRange &range, const TSeat &seat);
bool NextSeatInRange(const TSeatRange &range, TSeat &seat);

#endif /*_SEATS_UTILS_H_*/

