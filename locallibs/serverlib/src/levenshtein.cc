#if HAVE_CONFIG_H
#endif

#include <vector>
//#include <iostream>

#include "levenshtein.h"

/* �����ﭨ� ������⥩�� �� a �� b */
unsigned LevenshteinDistance(
        const std::string& a,
        const std::string& b,
        bool (*isEqual)(char a, char b))
{
    /* 
     * ��אַ㣮�쭠� ����� ࠧ��஬ (a.size() + 1) x (b.size() + 1).
     * ������� ������ � ����樨 i, j ᮮ⢥����� ����ﭨ� ������⥩��
     * ����� a.substr(0, i) � b.substr(0, j).
     *
     * ����� � ���������� a.substr(0, i) ������祭� ��� a(i),
     * b.substr(0, j) ᮮ⢥��⢥��� b(j).
     */
    std::vector<std::vector<unsigned> > m(
            a.size() + 1,
            std::vector<unsigned>(b.size() + 1, 0));

    /* ���樠�����㥬 �㫥��� ��ப� � �㫥��� �⮫��� ������ ����ﭨ�.
     *
     * �⮡� ������� ��ப� ������ N �� ���⮩ ��ப�, �㦭� �������� N ᨬ�����.
     * �⮡� ������� ������ ��ப� �� ��ப� ������ N, �㦭� 㤠���� N ᨬ�����.
     * ����� ��ࠧ��, �㫥��� ��ப� � �⮫��� ���� ᮤ�ঠ�� �ࠢ���� ���⠭樨.
     */
    for (size_t i = 0; i <= a.size(); ++i)
        m.at(i).at(0) = i;
    for (size_t j = 0; j <= b.size(); ++j)
        m.at(0).at(j) = j;

    /* ������塞 �� ��⠫�� ������ ������ */
    for (size_t i = 1; i <= a.size(); ++i)
        for (size_t j = 1; j <= b.size(); ++j) {
            const bool eq = isEqual ?
                isEqual(a.at(i - 1), b.at(j - 1)) :
                a.at(i - 1) == b.at(j - 1);

            /* �����ﭨ� ����� a(i) � b(j - 1) 㦥 �����⭮,
             * b(j - 1) ����� ������� �� b(j) 㤠������ ������ ᨬ����.
             */
            const unsigned del = m.at(i).at(j - 1) + 1;
            /* �����ﭨ� ����� a(i - 1) � b(j) 㦥 �����⭮,
             * a(i) ����� ������� �� a(i - 1) ����������� ������ ᨬ����.
             */
            const unsigned ins = m.at(i - 1).at(j) + 1;
            /* �����ﭨ� ����� a(i - 1) � b(j - 1) 㦥 �����⭮,
             * � �ᯮ�짮������ ������ ����ﭨ� ����� a(i) � b(j)
             * ����� �� �������.
             */
            const unsigned sub = m.at(i - 1).at(j - 1) + (eq ? 0 : 1);

            m.at(i).at(j) = std::min(del, std::min(ins, sub));
        }

    return m.at(a.size()).at(b.size());
}

/* �����ﭨ� ������-������⥩�� �� a �� b                */
/* (�� ��� � ������⥩��, �� �������� ���⠬� ��� �㪢� - *
 *�� ���� ��ࠢ�����, � �� ���)                          */
unsigned DamerauLevenshteinDistance(
        const std::string& a,
        const std::string& b)
{
    /* 
     * ��אַ㣮�쭠� ����� ࠧ��஬ (a.size() + 1) x (b.size() + 1).
     * ������� ������ � ����樨 i, j ᮮ⢥����� ����ﭨ� ������⥩��
     * ����� a.substr(0, i) � b.substr(0, j).
     *
     * ����� � ���������� a.substr(0, i) ������祭� ��� a(i),
     * b.substr(0, j) ᮮ⢥��⢥��� b(j).
     */
    std::vector<std::vector<unsigned> > m(
            a.size() + 1,
            std::vector<unsigned>(b.size() + 1, 0));

    /* ���樠�����㥬 �㫥��� ��ப� � �㫥��� �⮫��� ������ ����ﭨ�.
     *
     * �⮡� ������� ��ப� ������ N �� ���⮩ ��ப�, �㦭� �������� N ᨬ�����.
     * �⮡� ������� ������ ��ப� �� ��ப� ������ N, �㦭� 㤠���� N ᨬ�����.
     * ����� ��ࠧ��, �㫥��� ��ப� � �⮫��� ���� ᮤ�ঠ�� �ࠢ���� ���⠭樨.
     */
    for (size_t i = 0; i <= a.size(); ++i)
        m.at(i).at(0) = i;
    for (size_t j = 0; j <= b.size(); ++j)
        m.at(0).at(j) = j;

    /* ������塞 �� ��⠫�� ������ ������ */
    for (size_t i = 1; i <= a.size(); ++i)
        for (size_t j = 1; j <= b.size(); ++j) {
            const unsigned ReplaceCost =  (a.at(i - 1) == b.at(j - 1)) ? 0 : 1;

            /* �����ﭨ� ����� a(i) � b(j - 1) 㦥 �����⭮,
             * b(j - 1) ����� ������� �� b(j) 㤠������ ������ ᨬ����.
             */
            const unsigned del = m.at(i).at(j - 1) + 1;
            /* �����ﭨ� ����� a(i - 1) � b(j) 㦥 �����⭮,
             * a(i) ����� ������� �� a(i - 1) ����������� ������ ᨬ����.
             */
            const unsigned ins = m.at(i - 1).at(j) + 1;
            /* �����ﭨ� ����� a(i - 1) � b(j - 1) 㦥 �����⭮,
             * � �ᯮ�짮������ ������ ����ﭨ� ����� a(i) � b(j)
             * ����� �� �������.
             */
            const unsigned sub = m.at(i - 1).at(j - 1) + ReplaceCost;

            m.at(i).at(j) = std::min(del, std::min(ins, sub));
            
            /* � ��� ��� ���� �� ����⠭���� - �� ���� ��������� */
            if(i > 1 && j > 1 )
            {
             if(a.at(i-1) == b.at(j-2) && a.at(i-2) == b.at(j-1))
             {
              m.at(i).at(j) = std::min( m.at(i).at(j),  m.at(i - 2).at(j - 2) + ReplaceCost);
             }
            }
        }

    return m.at(a.size()).at(b.size());
}

/* �����ﭨ� ������-������⥩�� �� a �� b                */
/* (�� ��� � ������⥩��, �� �������� ���⠬� ��� �㪢� - *
 *�� ���� ��ࠢ�����, � �� ���)                          */
unsigned DamerauLevenshteinDistance_lim(
        const std::string& a,
        const std::string& b,
        const unsigned max_distance)
{
//    std::cout << "a='" << a << "', b='" << b << "'\n";
    /* 
     * ��אַ㣮�쭠� ����� ࠧ��஬ (a.size() + 1) x (b.size() + 1).
     * ������� ������ � ����樨 i, j ᮮ⢥����� ����ﭨ� ������⥩��
     * ����� a.substr(0, i) � b.substr(0, j).
     *
     * ����� � ���������� a.substr(0, i) ������祭� ��� a(i),
     * b.substr(0, j) ᮮ⢥��⢥��� b(j).
     */
    
    if(max_distance == 0) 
    {
     return ( a==b ? 0 : 1 );
    }
    
    unsigned mmax = max_distance + 1;

    /* 㤮���� �⮡� ��ࢠ� ��ப� �뫠 ������� */
    std::string const &str1 = (a.size() > b.size()) ? a : b;
    std::string const &str2 = (a.size() > b.size()) ? b : a;

    unsigned len1 = str1.size();
    unsigned len2 = str2.size();

    if(str1.empty() && !str2.empty()) {
     return ((len2 > mmax) ? mmax : len2 );
    }

    if(!str1.empty() && str2.empty()) {
     return ((len1 > mmax) ? mmax : len1 );
    }
    
    if((len1 - len2) > max_distance) 
    {
      return mmax;
    }
    
    std::vector<std::vector<unsigned> > m(
            len1 + 1,
            std::vector<unsigned>(len2 + 1, 0));

    /* ���樠�����㥬 �㫥��� ��ப� � �㫥��� �⮫��� ������ ����ﭨ�.
     *
     * �⮡� ������� ��ப� ������ N �� ���⮩ ��ப�, �㦭� �������� N ᨬ�����.
     * �⮡� ������� ������ ��ப� �� ��ப� ������ N, �㦭� 㤠���� N ᨬ�����.
     * ����� ��ࠧ��, �㫥��� ��ப� � �⮫��� ���� ᮤ�ঠ�� �ࠢ���� ���⠭樨.
     */
    for (size_t i = 0; i <= len1; ++i)
        m.at(i).at(0) = (i > mmax) ? mmax : i;
    for (size_t j = 0; j <= len2; ++j)
        m.at(0).at(j) = (j > mmax) ? mmax : j;
        
    unsigned min1=0, min2=0;

    /* ������塞 �� ��⠫�� ������ ������ */
    for (size_t i = 1; i <= len1 && (min1<mmax || min2<mmax); ++i)
    {
        if(i>1) 
        {
         min2 = min1;
         min1 = m.at(i).at(0);
        }
//std::cout << "min1 = " << min1 << ", min2=" << min2 << "\n\n";
        
        for (size_t j = 1; j <= len2; ++j) 
        {
            unsigned df = (i>j) ? (i-j) : (j-i);
            if(df > mmax)
            {
             m.at(i).at(j) = mmax;
             if(min1 > mmax) min1 = mmax;
//{
// std::cout << "\n============================================================\ni=" << i << ", j=" << j << "\n============================================================\n";
// for(size_t i1 = 0; i1 <= len1; ++i1) { for(size_t j1 = 0; j1 <= len2; ++j1) std::cout << m.at(i1).at(j1) << '\t'; std::cout << "\n"; }
//}
             continue;
            }
            
            const unsigned ReplaceCost =  (str1.at(i - 1) == str2.at(j - 1)) ? 0 : 1;
            /* �����ﭨ� ����� a(i) � b(j - 1) 㦥 �����⭮,
             * b(j - 1) ����� ������� �� b(j) 㤠������ ������ ᨬ����.
             */
            const unsigned del = m.at(i).at(j - 1) + 1;
            /* �����ﭨ� ����� a(i - 1) � b(j) 㦥 �����⭮,
             * a(i) ����� ������� �� a(i - 1) ����������� ������ ᨬ����.
             */
            const unsigned ins = m.at(i - 1).at(j) + 1;
            /* �����ﭨ� ����� a(i - 1) � b(j - 1) 㦥 �����⭮,
             * � �ᯮ�짮������ ������ ����ﭨ� ����� a(i) � b(j)
             * ����� �� �������.
             */
            const unsigned sub = m.at(i - 1).at(j - 1) + ReplaceCost;

            m.at(i).at(j) = std::min(del, std::min(ins, sub));
            
            /* � ��� ��� ���� �� ����⠭���� - �� ���� ��������� */
            if(i > 1 && j > 1 )
            {
             if(str1.at(i-1) == str2.at(j-2) && str1.at(i-2) == str2.at(j-1))
             {
              m.at(i).at(j) = std::min( m.at(i).at(j),  m.at(i - 2).at(j - 2) + ReplaceCost);
             }
            }
            
            if(min1 > m.at(i).at(j))
            {
             min1 = m.at(i).at(j);
            }
//{
// std::cout << "\n============================================================\ni=" << i << ", j=" << j << "\n============================================================\n";
// for(size_t i1 = 0; i1 <= len1; ++i1) { for(size_t j1 = 0; j1 <= len2; ++j1) std::cout << m.at(i1).at(j1) << '\t'; std::cout << "\n"; }
//}

        }
    }
    
//std::cout << "min1 = " << min1 << ", min2=" << min2 << ", mmax=" << mmax << "\n\n";
    if(min1>=mmax && min2>=mmax) {
//std::cout << "two strings are already mmax'ed\n\n\n";
     return mmax;
    }
    else
    {
//std::cout << "length = m[" << len1 << "][" << len2 << "] = " << m.at(len1).at(len2) << "\n\n\n";
     return m.at(len1).at(len2);
    }
}


#ifdef XP_TESTING
#define NICKNAME "DMITRYVM,KHONOV"
#include "xp_test_utils.h"
#include "checkunit.h"
namespace
{

START_TEST(Check_LevenshteinDistance)
{
    static const struct {
        const char* a;
        const char* b;
        unsigned expectedDistance;
    } variants[] = {
        { "", "", 0 },
        { "a", "", 1 },
        { "", "a", 1 },
        { "ab", "ac", 1 },
        { "ababaca", "aba", 4 },
        { "sirena", "Sirea", 2 },
        { "sirena", "sirena", 0 },
        { "sirena", "sierna", 2 },
        { "��� ��� �� ��� � �ᠫ� ����", "��� ���� �� ��� ᮠ᫠ ��襪", 10}
    };

    for (size_t i = 0; i < sizeof(variants) / sizeof(variants[0]); ++i) {
        const unsigned d = LevenshteinDistance(variants[i].a, variants[i].b);
        fail_unless(d == variants[i].expectedDistance, "LevenshteinDistance(%s, %s) == %u, %u expected",
                variants[i].a, variants[i].b, d, variants[i].expectedDistance);
    }
}
END_TEST;

START_TEST(Check_DamerauLevenshteinDistance)
{
    static const struct {
        const char* a;
        const char* b;
        unsigned expectedDistance;
    } variants[] = {
        { "", "", 0 },
        { "a", "", 1 },
        { "", "a", 1 },
        { "ab", "ac", 1 },
        { "ababaca", "aba", 4 },
        { "sirena", "Sirea", 2 },
        { "sirena", "sirena", 0 },
        { "sirena", "sierna", 1 },
        { "��� ��� �� ��� � �ᠫ� ����", "��� ���� �� ��� ᮠ᫠ ��襪", 8}
    };

    for (size_t i = 0; i < sizeof(variants) / sizeof(variants[0]); ++i) {
        const unsigned d = DamerauLevenshteinDistance(variants[i].a, variants[i].b);
        fail_unless(d == variants[i].expectedDistance, "LevenshteinDistance(%s, %s) == %u, %u expected",
                variants[i].a, variants[i].b, d, variants[i].expectedDistance);
    }
}
END_TEST;

START_TEST(Check_DamerauLevenshteinDistance_lim)
{
    static const struct {
        const char* a;
        const char* b;
        unsigned maxDistance;
        unsigned expectedDistance;
    } variants[] = {
        { "", "", 3, 0 },
        { "a", "", 3, 1 },
        { "", "a", 3, 1 },
        { "ab", "ac", 3, 1 },
        { "ababaca", "aba", 3, 4 },
        { "sirena", "Sirea", 3, 2 },
        { "sirena", "sirena", 3, 0 },
        { "sirena", "sierna", 3, 1 },
        { "��� ��� �� ��� � �ᠫ� ����", "lalalalalalalalalalalalalalalala", 3, 4},
        { "��� ��� �� ��� � �ᠫ� ����", "��� ���� �� ��� ᮠ᫠ ��襪", 12, 8},
        { "��� ��� �� ��� � �ᠫ� ����", "��� ���� �� ��� ᮠ᫠ ��襪",  6, 7}
    };

    for (size_t i = 0; i < sizeof(variants) / sizeof(variants[0]); ++i) {
        const unsigned d = DamerauLevenshteinDistance_lim(variants[i].a, variants[i].b, variants[i].maxDistance);
        fail_unless(d == variants[i].expectedDistance, "LevenshteinDistance(%s, %s, %u) == %u, %u expected",
                variants[i].a, variants[i].b, variants[i].maxDistance, d, variants[i].expectedDistance);
    }
}
END_TEST;

static void StartTests()
{
} 

static void FinishTests()
{
}

#define SUITENAME "dvm_ai_utils"
TCASEREGISTER(StartTests, FinishTests)
    ADD_TEST(Check_LevenshteinDistance);
    ADD_TEST(Check_DamerauLevenshteinDistance);
    ADD_TEST(Check_DamerauLevenshteinDistance_lim);
TCASEFINISH

} /* namespace */
#endif // XP_TESTING

