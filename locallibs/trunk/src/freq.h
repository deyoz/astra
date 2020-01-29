#ifndef SERVERLIB_FREQ_H
#define SERVERLIB_FREQ_H

#include <string>
#include <bitset>
#include <boost/optional.hpp>

namespace boost { namespace gregorian { class date; } }

class Freq
{
public:
    Freq();
    Freq(const std::bitset<7>&);
    Freq(const std::string&);
    Freq(const boost::gregorian::date&, const boost::gregorian::date&);

    bool empty() const;
    bool full() const;
    bool hasDayOfWeek(unsigned short) const;
    std::string normalString() const;
    std::string str() const;
    Freq shift(int days) const;

    bool operator==(const Freq&) const;
    bool operator<(const Freq&) const;
    bool operator>(const Freq&) const;
    Freq operator&(const Freq&) const;
    Freq operator-(const Freq&) const;
    friend std::ostream& operator<<(std::ostream& str, const Freq&);
private:
    std::bitset<7> days_;
    friend std::ostream& operator<<(std::ostream& str, const Freq&);
};
boost::gregorian::date nextDate(const boost::gregorian::date&, const Freq&);

boost::optional<Freq> freqFromStr(const std::string&);
Freq logicalOr(const Freq& lhs, const Freq& rhs);

class frequency
{
    enum {  week_length=7  };
    bool days[week_length];
    bool valid_;
  public:
    frequency() : valid_(false) {  for(int i=0; i<week_length; ++i) days[i]=false;  }
    bool isValid() const {  return valid_;  }
    std::string toString(char delimeter='.') const;
    int toInt() const;
    bool includes(int day) const;
    bool intersects(const frequency& f) const;
    frequency intersection(const frequency& f) const;
    static frequency fromString(const std::string& in, char delimeter='.');
    static frequency fromInt(int bits);
};

inline frequency frequency::fromString(const std::string& in, char delimeter)
{
  frequency f;
  f.valid_=false;
  for(std::string::const_iterator i=in.begin(); i!=in.end(); ++i)
  {
      if(*i!=delimeter && (*i<'1' || *i>'7'))
          return f;
      f.days[*i-'1']=true;
  }
  f.valid_=true;
  return f;
}

inline frequency frequency::fromInt(int bits)
{
  frequency f;
  f.valid_=false;
  if(bits >= 1<<(week_length+1))  return f;
  for(int i=0; i<week_length; ++i)
      f.days[i] = bits & 1<<i;
  f.valid_=true;
  return f;
}

inline std::string frequency::toString(char delimeter) const
{
  if(!valid_)  return "";
  char buf[week_length+1]={0};
  for(int i=0; i<week_length; ++i)
      buf[i] = days[i] ? i+'1' : delimeter;
  return buf;
}

inline int frequency::toInt() const
{
  int bits=0;
  for(int i=0; i<week_length; ++i)
      bits |= days[i] << i;
  return bits;
}

inline bool frequency::intersects(const frequency& f) const
{
  for(int i=0; i<week_length; ++i)
      if(days[i] && f.days[i])
          return true;
  return false;
}

inline frequency frequency::intersection(const frequency& f) const
{
  frequency ret;
  for(int i=0; i<week_length; ++i)
      ret.days[i] = days[i] && f.days[i];
  return ret;
}

inline bool frequency::includes(int day) const
{
  return day>=0 && day<week_length && days[day];
}
/*
inline std::list<boost::gregorian::date> get_matching_days(boost::gregorian::date_period, const frequency& f) const
{
  std::list<boost::gregorian::date> days;
  for(boost::gregorian::day_iterator pos=period.begin(); pos!=period.end(); ++pos)
      if(f.includes(pos->day_of_week().as_number()))
          days.push_back(pos->day());
  return days;
}
*/
#endif /* SERVERLIB_FREQ_H */
