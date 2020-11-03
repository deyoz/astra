#pragma once

#ifndef NICKNAME
#define NICKNAME "NVR"
#endif

#include <iostream>


enum CoutPriority
{
    COUT_ERROR=1,
    COUT_WARNING=2,
    COUT_INFO=3,
    COUT_DEBUG=4
};


//�뢮� � stdout
//�� �᫮��� ⮣�, �� level �����(���� ࠢ��), 祬 ���祭�� 㪠������ � ��६����� ���㦥��� STDOUT_CUTLOGGING
//�ᯮ������ �㭪�� LogCout(const CoutPriority level)
//�ਬ�� LogCout(COUT_INFO) << "123" << std::endl;
class CoutLogger
{
public:

    CoutLogger& operator<< (std::ostream& (*manip)(std::ostream&));

    template <class T>
    CoutLogger& operator<<(const T& t)
    {
        if(can_write()) std::cout << t;
        return *this;
    }

    friend CoutLogger LogCout(const CoutPriority level);

private:
    const int level_;

    CoutLogger(const CoutPriority level);
    bool can_write() const;

    CoutLogger() = delete;

};

CoutLogger LogCout(const CoutPriority level);

