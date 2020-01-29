#ifndef __OCI_EXCEPTION_H_
#define __OCI_EXCEPTION_H_

#include <string>
#include "exception.h"

namespace OciCpp
{

template <typename> struct sel_data;
/**
  * @class ociexception
  * @brief exceptions of this class may be thrown when using CursCtl
  */
class ociexception : public comtech::Exception
{
    std::string backtrace;
    std::string main_text;
    int ora_err;
    std::string ora_text;
    std::string what_str;

  public:
    virtual ~ociexception() = default;
    virtual char const *what() const noexcept;
    /**
     * @return Oracle error text */
    std::string const & getOraText() const noexcept { return ora_text; }
    /**
     * @return Oracle error code */
    int sqlErr() const noexcept { return ora_err; }

    ociexception(const std::string &msg);
    ociexception(const std::string &msg, int err_code, std::string const &err_msg);
};

struct UniqConstrException : public ociexception {
    UniqConstrException(std::string const& msg, std::string const &err_msg);
};

} // namespace OciCpp

#endif // __OCI_EXCEPTION_H_
