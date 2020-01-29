#ifndef LOCALTIME_HPP_
#define LOCALTIME_HPP_

#include <vector>
#include <string>

namespace boost {
    namespace posix_time {
        class ptime;
        class time_duration;
    }
    namespace gregorian {
        class date;
    }
}

/* ����� LocalTime �।��⠢��� ����� ����室���� ��⮤�� �
 * ����᪨� �㭪権 ��� �������樨 �६�� ����� �६���묨 ������.
 * ����� 㬥�� ࠡ���� � ᫥���騬� �ଠ⠬� �६���:
 * 1) boost::posix_time::ptime  
 * 2) �譠� ��ப� �ଠ� YYYYMMDDHHMMSS (14 ᨬ�����)
 * 3) int64_t, ������⢮ ᥪ㭤 � 01.01.1970 0:00:00 (�᫨ �६�
 *    �����쭮�, � ������⢮ ᥪ㭤 � 01.01.1970 0:00:00, ���஥
 *    ��諮 �� � GMT, �᫨ �� ⠬ �뫮 ⠪�� �६� �� ���)
 *    
 * ����� ����� ᮤ�ন� �� ����᪨� �㭪権 ��� �������樨 ࠧ���
 * �ଠ⮢ �६��� ��� � ��㣠.
 */

/***********************************************************
  EXAMPLES:
1) ���� ��᪮�᪮� �६�, ���� �६� � ��૨��:
   int berlin_time = LocalTime(moscow_time,"Europe/Moscow").setZone("Europe/Berlin").getLocalTime();
   ��� ⠪:
   boost::posix_time::ptime berlin_time = LocalTime(moscow_time,"Europe/Moscow")
                                            .setZone("Europe/Berlin").getBoostLocalTime();

2) ���� �६� ��ࠢ����� �� �६��� ����� � �६� �ਡ��� �� �६��� ������, ���� �६� � ���:
   LocalTime tokyo(tokyo_time,"Asia/Tokyo");
   LocalTime sydney(sydney_time,"Australia/Sydney");
   int travel_time = sydney - tokyo;
   
***********************************************************/

/* � �裡 � ��ॢ���� �ᮢ, ��-�����, �� �ᥣ�� ����� �����쭮��
   �६��� ���⠢��� � ᮮ⢥��⢨� ⮫쪮 ���� �६� �� GMT
   (���ਬ��, �����쭮� �६� 2:30 ����砥��� ������, �᫨ ��� �
   3:00 ��ॢ���� �� �� �����) � �� ������ �����쭮� �६� �����
   �������� (���ਬ��, ���������� �६� 2:30 �᫨ ��� � 2:00 �뫨
   ��ॢ����� �� �� ���।).

   �᫨ �६� ��ॢ���� �� �� ���। � 2:00:00, � ��᫥ 2:00:00 ����
   3:00:01, � �����쭮� �६� � �஬���⪥ [2:00:01, 3:00:00] ����������.

   �᫨ �६� ��ॢ���� �� �� ����� � 3:00:00, � ��᫥ 3:00:00 ����
   2:00:01, � �� �����쭮� �६� � �஬���⪥ [2:00:01, 3:00:00] 
   ����� ᮮ⢥��⢮���� ��� �६���� �� GMT.

   � ������������ ������ ��࠭� ᫥���饥 ���������:

   ����� 28.03.2010 � 2:00:00 �६� ��ॢ���� �� �� ���।, �.�. ��
   3:00:00 (�. �. gmtoff ᤢ������� � +3 �� +4 � 23:00:00 27.03 �� GMT)

   **local --> GMT**
   28.03 2:00:00 --> 27.03 23:00:00 (GMT+3) // � 2:00:00 �६� �� ��஥
   28.03 3:00:01 --> 27.03 23:00:01 (GMT+4) // � �१ ᥪ㭤� -- 㦥 �����

   // ��ᬮ��� �� ������������� �६�� [2:00:01, 3:00:00], ⠪��
   // �室�� ����� ��ࠡ��뢠����. ���ਬ��, �����४⭮� �६� 2:05 -- ��
   // �६�, ���஥ �� 5 ����� �����, 祬 ��᮫�⭮ ���४⭮� 2:00.
   28.03 2:00:01 --> 27.03 23:00:01 (GMT+3)
   28.03 3:00:00 --> 28.03  0:00:00 (GMT+3) // !!! 3:00:00 ���� �� �� *�����*, 祬 3:00:01 !!!


   ����� 31.10.2010 � 3:00:00 �६� ��ॢ���� �� �� �����, �.�. �� 2:00:00
   (�. �. gmtoff ᤢ������� � +4 �� +3 � 23:00:00 30.10 �� GMT)

   **local --> GMT**
   31.10 2:00:00 --> 30.10 22:00:00 (GMT+4) // � 2:00:00 �६� �� ��஥, � �१ ᥪ㭤�
                                            // �㤥� ����� � ��������� ���������筮���
   31.10 2:00:01 --> 30.10 23:00:01 (GMT+3) // �� ���������筮�� �롨ࠥ��� �६� *��᫥* ��ॢ��� ��५��
   31.10 3:00:01 --> 31.10  0:00:01 (GMT+3) // ����� ���������筮�� 㦥 ���

   **GMT  --> local**
   30.10 22:59:59 --> 31.10 2:59:59 (GMT+4) // setLocalTime(31.10 2:59:59) �� ���� 30.10 22:59:59
   30.10 23:00:00 --> 31.10 3:00:00 (GMT+4) // �������筮
   30.10 23:00:01 --> 31.10 2:00:01 (GMT+3)
*/

class TimeZone;
bool checkTimeZone(const std::string& zone);

class LocalTime
{
    LocalTime(int64_t a, const TimeZone * p) : zone(p), gmt(a) {}; 
private:
    const TimeZone * zone;
    int64_t gmt; // �६� � UTC/GMT
public: 
    // ���������� �ਭ����� � ����⢥ ��㬥�� �����쭮� �६� �
    // ��ப�-�����䨪��� �६����� ����
    // �᫨ ��ப� �� 㪠����, �।����������, �� ������� � ���� GMT
    LocalTime(const int64_t time = 0);
    LocalTime(const boost::posix_time::ptime &);
    LocalTime(const char * YYYYMMDDHHMMSS);
    LocalTime(const std::string& YYYYMMDDHHMMSS);
    LocalTime(const int64_t, const char * tzStr);

    LocalTime(const int64_t, const std::string& tzStr);
    LocalTime(const boost::posix_time::ptime & time, const char * tzStr);
    LocalTime(const boost::posix_time::ptime& time, const std::string& tzStr);

    LocalTime(const char * YYYYMMDDHHMMSS, const char * s);
    LocalTime(const std::string& YYYYMMDDHHMMSS, const std::string& s);

    // ࠧ��� ����� ���� ��⠬� � ᥪ㭤��
    friend int64_t operator-(const LocalTime &, const LocalTime &); 

    // 㬥����� ���� �� �������� ������⢮ ᥪ㭤 ��� �� boost::posix_time::time_duration
    LocalTime operator-(const boost::posix_time::time_duration &) const;
    LocalTime operator-(const int64_t) const;
    LocalTime & operator-=(const boost::posix_time::time_duration &);
    LocalTime & operator-=(const int64_t);

    // 㢥����� ���� �� �������� ������⢮ ᥪ㭤 ��� �� boost::posix_time::time_duration
    LocalTime operator+(const boost::posix_time::time_duration &) const;
    LocalTime operator+(const int64_t) const; 
    LocalTime & operator+=(const boost::posix_time::time_duration &);
    LocalTime & operator+=(const int64_t);

    friend bool operator<(const LocalTime &, const LocalTime &); 
    friend bool operator==(const LocalTime &, const LocalTime &); 

    // ��������� �६����� ���� (setZone) �� ����� �� �࠭���� ����� �६�,
    // � ����� *⮫쪮* �� ��������� ��⮤�� ᥬ���⢠ s
    LocalTime & setZone(const char *);
    LocalTime & setZone(const std::string&);

    LocalTime & setLocalTime(const boost::posix_time::ptime &);
    LocalTime & setLocalTime(const char * YYYYMMDDHHMMSS);
    LocalTime & setLocalTime(const std::string& YYYYMMDDHHMMSS);
    LocalTime & setLocalTime(const int64_t);
    LocalTime & setGMTime(const boost::posix_time::ptime &);
    LocalTime & setGMTime(const char * YYYYMMDDHHMMSS);
    LocalTime & setGMTime(const std::string& YYYYMMDDHHMMSS);
    LocalTime & setGMTime(const int64_t);

    boost::posix_time::ptime getBoostLocalTime() const;
    int64_t getLocalTime() const;
    std::string getLocalTimeStr() const; // ���� �६� � ���� YYYYMMDDHHMMSS
    boost::posix_time::ptime getBoostGMTime() const; 
    int64_t getGMTime() const { return gmt; }
    std::string getGMTimeStr() const;// ���� �६� � ���� YYYYMMDDHHMMSS
};

// �㭪樨, ����������騥 ࠧ�� �ଠ�� �।�⠢����� �६��� ��� � ��㣠
// ����⢥���, �� �� �⮬ ��� �� ���� ��ࠧ�� �� ���뢠�� ������� �६���� ����
int64_t strToTime(const char * YYYYMMDDHHMMSS);
int64_t strToTime(const std::string& YYYYMMDDHHMMSS);
int64_t boostToTime(const boost::posix_time::ptime &);
std::string timeToStr(int64_t time); // ���� �६� � ���� YYYYMMDDHHMMSS
boost::posix_time::ptime timeToBoost(int64_t time);

std::string City2City(const std::string& from_time, const std::string& from_zone,const std::string& to_zone);
boost::posix_time::ptime City2City(boost::posix_time::ptime const &from_time, const std::string& from_zone,const std::string& to_zone);
std::string City2GMT(const std::string& time, const std::string& zone);
boost::posix_time::ptime City2GMT(const boost::posix_time::ptime& time, const std::string& zone);
std::vector<boost::gregorian::date> getZoneLeapDates(const std::string& zone,
        const boost::gregorian::date& start, const boost::gregorian::date& end, bool inUTC = true);

#endif /* LOCALTIME_HPP_ */
