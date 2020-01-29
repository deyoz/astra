#include <regex.h>
#include <regex>

#include "cc_censor.h"
#include "str_utils.h"
#include "isdigit.h"

#include <string.h>

#define NICKNAME "ROMAN"

#ifdef tst
#error do not use logging here
#endif

static bool is_luhn_valid(const char* in, size_t len);

// !!!!!!!!!!!!!!!!!
// DO NOT USE LogTrace, ProgTrace, tst, TST or any other trace funcs in this function
// !!!!!!!!!!!!!!!!!
bool censure(char *buff)
{
    static regex_t* reg_ = nullptr;
    if(!reg_) {
        reg_ = new regex_t();
        regcomp(reg_,
            "(4[0-9]{15}"      // visa
            "|5[1-5][0-9]{14}" // master card
            "|50[0-9]{13,17}" // maestro
            "|5[6-9][0-9]{13,17}" // maestro
            "|6[0-9]{14,18}"    // maestro
            "|3[47][0-9]{13}"  // american expr
            ")", REG_EXTENDED);
    }

    bool mask_applied = false;
    int lmp = 0; /*last match pos*/
    regmatch_t pmatch[1];
    while(!regexec(reg_, buff + lmp, 1, pmatch, 0))
    {
        char* cc_num = buff + lmp + pmatch[0].rm_so;
        const size_t cc_len = pmatch[0].rm_eo - pmatch[0].rm_so;
        if(cc_len < 15)
            ;// hmm
        else if(is_luhn_valid(cc_num, cc_len))
        {
            strncpy(cc_num + 6, "**********", cc_len - 10);
            mask_applied = true;
        }
        lmp = lmp + pmatch[0].rm_eo;
    }
    return mask_applied;
}

std::string censure(const std::string &buff)
{
    char cbuff[buff.length() + 1];
    memcpy(cbuff, buff.c_str(), buff.length() + 1);

    (void)censure(cbuff);
    return cbuff;
}

static bool is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

/*
1. Цифры проверяемой последовательности нумеруются справа налево.
2. Цифры, оказавшиеся на нечётных местах, остаются без изменений.
3. Цифры, стоящие на чётных местах, умножаются на 2.
4. Если в результате такого умножения возникает число больше 9, оно заменяется суммой цифр получившегося произведения — однозначным числом, то есть цифрой.
5. Все полученные в результате преобразования цифры складываются. Если сумма кратна 10, то исходные данные верны.
*/

static bool is_luhn_valid(const char* in, size_t len)
{
    unsigned int sum = 0;
    for(size_t i = 0; i < len; i++)
    {
        const int c = in[i] - '0';
        if(c < 0 or c > 9)
            return false;
        if((len - i) & 1) {
            sum += c;
        } else {
            const auto x = 2 * c;
            sum += x > 9 ? x - 9 : x;
        }
    }
    return (sum % 10) == 0;
}

static bool is_luhn_valid(const std::string& in)
{
    return is_luhn_valid(in.data(), in.size());
}

static std::string get_numbers(const std::string& in)
{
    std::string out;
    std::copy_if(in.begin(), in.end(), std::back_inserter(out), [] (char c) { return is_digit(c); });
    return out;
}

std::vector<string_range> extractCardNumbers(const std::string& in, bool strictCheck)
{
    static const std::regex rx(
        "("
        "4[0-9]{15}"            // visa
        "|5[1-5][0-9]{14}"      // master card
        "|50[0-9]{13,17}"       // maestro
        "|5[6-9][0-9]{13,17}"   // maestro
        "|6[0-9]{14,18}"        // maestro
        "|3[47][0-9]{13}"       // american expr
        ")"
    );

    static const std::regex rx2(
        "([0-9][0-9 -]{9,}[0-9]{4})"
    );

    std::vector<string_range> out;

    auto sbeg = std::sregex_iterator(in.begin(), in.end(), rx2);
    for (auto i = sbeg, e = std::sregex_iterator(); i != e; ++i)  {
        const std::smatch& m = *i;
        if (m[1].matched) {
            const std::string nums = get_numbers(m[1]);
            if (std::regex_match(nums, rx)) {
                if (!strictCheck || is_luhn_valid(get_numbers(m[1]))) {
                    out.emplace_back(
                        std::distance(in.begin(), m[1].first),
                        std::distance(m[1].first, m[1].second)
                    );
                }
            }
        }
    }

    return out;
}

bool containsCardNumbers(const std::string& in, bool strictCheck)
{
    return !extractCardNumbers(in, strictCheck).empty();
}

static std::string replace_cc(const std::string& in, char replacement)
{
    std::string rpl = in;

    const size_t numCnt = std::count_if(
        in.begin(), in.end(), [] (char c) { return is_digit(c); }
    );

    size_t n = 0;
    for (auto i = rpl.begin(), e = rpl.end(); i != e; ++i) {
        if (is_digit(*i)) {
            if (n == 0 || (n >= 4 && n < numCnt - 4)) {
                *i = replacement;
            }
            ++n;
        }
    }

    return rpl;
}

bool maskString(std::string& s, bool strictCheck, char c,
        std::function<void (const std::string&, const std::string&)> trace)
{
    std::string out(s);
    for (const auto& v : extractCardNumbers(out, strictCheck)) {
        const std::string cn = out.substr(v.first, v.second);
        if (!cn.empty()) {
            const std::string rpl = replace_cc(cn, c);
            out.replace(v.first, v.second, rpl);
            if (trace) {
                trace(cn, rpl);
            }
        }
    }
    const bool changed = s != out;
    s = out;
    return changed;
}

std::string maskCardNumber(const std::string& s, char mask_char)
{
    std::string res;

    const size_t D_CARD_TYPE_LEN=2;

    // Find begin of number
    size_t beg_num=(s.length()>0 && ISDIGIT(s[0]))?0:
      s.length()>=D_CARD_TYPE_LEN?D_CARD_TYPE_LEN:0;
    for(; beg_num<s.length() && s[beg_num]==' '; ++beg_num) ;
    // Find end of number
    size_t len_num=0;
    for(; beg_num+len_num<s.length() && s[beg_num+len_num]!=' ';++len_num) ;

    if (len_num>2)
    {
      //const size_t FULL_LEN=16;
      const size_t FULL_LEN=len_num>19?19:len_num;
      const size_t MAX_BEGIN_LEN=6;
      const size_t MAX_END_LEN=4;
      const size_t MIN_XXX_LEN=3;


      size_t beg_len=0;
      for(; beg_len<MAX_BEGIN_LEN && beg_len<len_num &&
        ISDIGIT(s[beg_num+beg_len]); ++beg_len) ;

      size_t end_len=0;
      for(; end_len<MAX_END_LEN && end_len<len_num &&
        ISDIGIT(s[beg_num+len_num-end_len-1]); ++end_len) ;

      if (beg_len+end_len+MIN_XXX_LEN>len_num)
      {
        bool digits_only=true;
        if (beg_len+end_len<len_num)
          for(size_t i=beg_len; digits_only && i<len_num-end_len; ++i)
            digits_only=ISDIGIT(s[beg_num+i]) ;


        if (digits_only)
        {
          int diff=len_num-beg_len-end_len;

          if (diff<0)
          {
            size_t end_diff=0-diff/2;
            if(end_diff>=end_len)
              end_diff=end_len>0?end_len-1:0;

            size_t beg_diff=0-diff-end_diff;
            if(beg_diff>=beg_len)
              beg_diff=beg_len>0?beg_len-1:0;

            beg_len-=beg_diff;
            end_len-=end_diff;

            diff=len_num-beg_len-end_len;
          }

          diff=MIN_XXX_LEN-diff;
          if (diff<0)
            diff=0;

          size_t end_diff=diff/2;
          if(end_diff>=end_len)
            end_diff=end_len>0?end_len-1:0;

          size_t beg_diff=diff-end_diff;
          if(beg_diff>=beg_len)
            beg_diff=beg_len>0?beg_len-1:0;

          beg_len-=beg_diff;
          end_len-=end_diff;
        }
      }
      int x_count=FULL_LEN-beg_len-end_len;
      if (x_count<(int)MIN_XXX_LEN)
        x_count=MIN_XXX_LEN;

      res=s.substr(0,beg_num+beg_len)+std::string(x_count,mask_char)+
        s.substr(beg_num+len_num-end_len);
    }
    else res=s;

    return res;
}

//#############################################################################
#ifdef XP_TESTING
#include <serverlib/checkunit.h>
#include <serverlib/xp_test_utils.h>

START_TEST(check_card_extract)
{
    struct Case {
        std::string input;
        std::string output;
        bool strict;
    };

    const std::vector<Case> cases = {
        { "4024 0071 7891 5186",  "4024 0071 7891 5186", true },
        { "4024-0071-7891-5186",  "4024-0071-7891-5186", true },
        { "AB5121674112995670C", "5121674112995670", true },
        { "AB 21512167411299567-0C", "", true },
        { "AB 21512167411299567-0C", "", false },
        { "AB 512167411299-5670C", "512167411299-5670", true },
        { "bla-bla UT 4024007178915186ABL", "4024007178915186", true },
        { "bla-bla UT4024007178925186 ABL", "", true },
        { "bla-bla UT4024007178925186 ABL AB5121674112995670C", "5121674112995670", true },
        { "bla-bla UT4024007178925186 ABL AB5121 6741 1299 5670C", "5121 6741 1299 5670", true },
        { "bla-bla UT4024007178925186 ABL AB5121674112995670C", "4024007178925186/5121674112995670", false }
    };

    unsigned int cn = 0;
    for (const Case& c : cases) {
        std::string vv;
        for (const auto& v : extractCardNumbers(c.input, c.strict)) {
            vv.append(vv.empty() ? 0 : 1, '/');
            vv.append(c.input.substr(v.first, v.second));
        }
        ++cn;
        fail_unless(c.output == vv, "%d: %s vs %s", cn, c.output.c_str(), vv.c_str());
    }
}
END_TEST

START_TEST(check_card_replace)
{
    const std::vector< std::pair < std::string, std::string > > cases = {
        { "4024 0071 7891 5186", "0024 0000 0000 5186" },
        { "AB5121674112995670C", "AB0121000000005670C" },
        { "bla-bla UT 4024007178915186ABL AND 4929948840644892ABC", "bla-bla UT 0024000000005186ABL AND 0929000000004892ABC" },
        { "bla-bla UT4024007178925186 ABL", "bla-bla UT4024007178925186 ABL" },
        { "bla-bla UT4024007178925186 ABL4929948840644892", "bla-bla UT4024007178925186 ABL0929000000004892" }
    };

    unsigned int cn = 0;
    for (const auto& c : cases) {
        std::string v = c.first;
        maskString(v, true, '0', {});
        ++cn;
        fail_unless(c.second == v, "%d: %s vs %s", cn, c.second.c_str(), v.c_str());
    }
}
END_TEST

#define SUITENAME "cardnumber"
TCASEREGISTER(0, 0)
{
    ADD_TEST(check_card_extract);
    ADD_TEST(check_card_replace);
}
TCASEFINISH

#endif //XP_TESTING
