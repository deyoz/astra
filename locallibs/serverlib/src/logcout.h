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


//Вывод в stdout
//При условии того, что level менъше(либо равно), чем значение указанное в переменной окружения STDOUT_CUTLOGGING
//Исполъзуется функция LogCout(const CoutPriority level)
//Пример LogCout(COUT_INFO) << "123" << std::endl;
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

