#pragma once

#include "oralib.h"

class TBagTagNumber
{
  public:
    std::string alpha_part;
    double numeric_part;
    int numeric_part_max_len;
    TBagTagNumber(const std::string &apart, double npart)
      :alpha_part(apart),numeric_part(npart),numeric_part_max_len(10) {}
    TBagTagNumber(const std::string &apart, double npart, int max_len)
      :alpha_part(apart),numeric_part(npart),numeric_part_max_len(max_len) {}
    bool operator < (const TBagTagNumber &no) const
    {
      if (numeric_part_max_len!=no.numeric_part_max_len)
        return numeric_part_max_len<no.numeric_part_max_len;
      if (alpha_part!=no.alpha_part)
        return alpha_part<no.alpha_part;
      return numeric_part<no.numeric_part;
    }
    bool operator == (const TBagTagNumber &no) const
    {
      return numeric_part_max_len == no.numeric_part_max_len &&
             alpha_part == no.alpha_part &&
             numeric_part == no.numeric_part;
    }
    double pack() const
    {
      double result;
      modf(numeric_part/1000.0,&result);
      return result;
    }
    double number_in_pack() const
    {
      return fmod(numeric_part, 1000.0);
    }
    bool equal_pack(const TBagTagNumber& tag) const
    {
      return alpha_part==tag.alpha_part &&
             numeric_part_max_len==tag.numeric_part_max_len &&
             pack()==tag.pack();
    }
    std::string str() const
    {
      std::ostringstream s;
      s.setf(std::ios::fixed);
      s << alpha_part
        << std::setw(numeric_part_max_len)
        << std::setfill('0') << std::setprecision(0)
        << numeric_part;
      return s.str();
    }
    std::string number_in_pack_str(int inc=0) const
    {
      std::ostringstream s;
      s.setf(std::ios::fixed);
      s << std::setw(3)
        << std::setfill('0') << std::setprecision(0)
        << number_in_pack()+inc;
      return s.str();
    }
};

void GetTagRanges(const std::multiset<TBagTagNumber> &tags,
                  std::vector<std::string> &ranges);   //ranges сортирован
std::string GetTagRangesStrShort(const std::multiset<TBagTagNumber> &tags);

std::string GetTagRangesStr(const std::multiset<TBagTagNumber> &tags);
std::string GetTagRangesStr(const TBagTagNumber &tag);

class TBagTagRange
{
  public:
    TBagTagRange(const std::string &apart, double npart_first, double npart_last, int max_len)
      : _alpha_part(apart),
        _numeric_part_first(npart_first),
        _numeric_part_last(npart_last),
        _numeric_part_max_len(max_len) {}

    const std::string& alpha_part()    const { return _alpha_part; }
    const double& numeric_part_first() const { return _numeric_part_first; }
    const double& numeric_part_last()  const { return _numeric_part_last; }
    const int& numeric_part_max_len()  const { return _numeric_part_max_len; }
  private:
    std::string _alpha_part;
    double _numeric_part_first;
    double _numeric_part_last;
    int _numeric_part_max_len;
};

class TGeneratedTags
{
  public:
    void clear()
    {
      _grp_id=boost::none;
      _tags.clear();
    }

    void clear(int grp_id)
    {
      _grp_id=grp_id;
      _tags.clear();
    }

    const std::set<TBagTagNumber>& tags() const { return _tags; }
    void useDeferred(int grp_id, int tag_count);
    void defer();
    void generate(int grp_id, int tag_count);
    void generateUsingDeferred(int grp_id, int tag_count);
    void trace(const std::string &where) const;

    static void cleanOldRecords(int min_ago);

  protected:
    void add(const TBagTagRange& range);

  private:
    boost::optional<int> _grp_id;
    std::set<TBagTagNumber> _tags;
};

void GetTagsByBagNum(int grp_id, int bag_num, std::multiset<TBagTagNumber> &tags);
void GetTagsByPool(int grp_id, int bag_pool_num , std::multiset<TBagTagNumber> &tags);
void GetTagsByPaxId(int pax_id, std::multiset<TBagTagNumber> &tags);
