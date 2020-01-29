#ifndef LIBTLG_EXPRESS_SENDER_H
#define LIBTLG_EXPRESS_SENDER_H

#include <string>
#include <boost/asio/io_service.hpp>

namespace telegrams
{
namespace express
{

class Sender
{
public:
    Sender(
            int supervisorSocket,
            const std::string& handlerName,
            const char* senderSocketName,
            const char* senderPortName
    );

    virtual ~Sender();

    int run();

private:
    void *pImpl_;
};

} // namespace express
} // namespace telegrams


#endif /* LIBTLG_EXPRESS_SENDER_H */

