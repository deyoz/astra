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
    bool Empty()
    {
      return *row==0 || *line==0;
    };

    TSeat& operator = ( const TSeat& seat )
    {
      if (this == &seat) return *this;
      strncpy(this->row,seat.row,sizeof(seat.row));
      strncpy(this->line,seat.line,sizeof(seat.line));
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
};

class TSeatRange : public std::pair<TSeat,TSeat>
{
  public:
    char rem[5];
    TSeatRange() : std::pair<TSeat,TSeat>()
    {
      *rem=0;
    };
    TSeatRange(TSeat seat1, TSeat seat2) : std::pair<TSeat,TSeat>(seat1,seat2)
    {
      *rem=0;
    };
    TSeatRange(TSeat seat1, TSeat seat2, const std::string &rem) : std::pair<TSeat,TSeat>(seat1,seat2)
    {
      strncpy(this->rem,rem.c_str(),sizeof(this->rem));
    };
    friend bool operator < ( const TSeatRange& range1, const TSeatRange& range2 )
    {
      return range1.first<range2.first;
    };
};

//функция кроме представления номера места возвращает кол-во мест
std::string GetSeatRangeView(const std::vector<TSeatRange> &ranges,
                             const std::string &format,
                             bool pr_lat,
                             int &seats);

std::string GetSeatRangeView(const std::vector<TSeatRange> &ranges,
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
bool SeatInRange(TSeatRange &range, TSeat &seat);
bool NextSeatInRange(TSeatRange &range, TSeat &seat);

#endif /*_SEATS_UTILS_H_*/

