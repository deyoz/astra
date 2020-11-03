#ifndef __DETAIL_H__
#define __DETAIL_H__

#include <vector>
#include <string>
#include <memory>

#include "text_message.hpp"

namespace detail
{

struct text_queue;

struct connection
{
    virtual void check() = 0;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    virtual void break_wait() = 0;
    virtual std::string listen(const std::vector<std::string>&) = 0;
    virtual std::shared_ptr<text_queue> create_text_queue(const std::string& , bool xact=true) = 0;
    virtual ~connection() {}
    virtual bool has_internal_recode() const { return false; }
};

struct text_queue:public std::enable_shared_from_this<text_queue>
{
    virtual jms::text_message dequeue(const jms::recepient&, const std::string&) = 0;

    virtual bool dequeue
    (
      jms::text_message& msg, unsigned wait_delay,
      const jms::recepient&, const std::string&
    ) = 0;

    virtual void enqueue(const jms::text_message&, const jms::recepients&) = 0;
    virtual ~text_queue() {}
};

}

#endif // __DETAIL_H__

