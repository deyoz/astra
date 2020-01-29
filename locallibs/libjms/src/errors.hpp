#ifndef _ERRORS_HPP_
#define _ERRORS_HPP_

#include <string>
#include <stdexcept>

namespace jms
{

class mq_error : public std::logic_error
{
   public:
     mq_error(int err_code, const std::string& desc) : logic_error(desc), err_code_(err_code)
     {
     }
     int get_errcode() const
     {
        return err_code_;
     }
   private:
     int err_code_;
};

}

#endif

