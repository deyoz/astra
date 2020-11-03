#ifndef _JMS_HPP_
#define _JMS_HPP_

#include <vector>
#include <string>
#include <memory>
#include "text_message.hpp"
#include "errors.hpp"

namespace detail
{

struct connection;
struct text_queue;

} // namespace detail

namespace jms
{

class text_queue;

class connection
{
  public:
    connection(const std::string&, bool = false);
    ~connection();
    void check();
    void commit();
    void rollback();
    void break_wait();
    text_queue create_text_queue(const std::string&, bool xact=true);
    std::string listen(const std::vector<std::string>&);
    bool has_internal_recode() const;
  private:
    connection(const connection&);
    connection& operator = (const connection&);
    std::shared_ptr<detail::connection> impl_;
};

typedef std::shared_ptr<connection> connection_ptr;

class text_queue
{
  public:
    ~text_queue();
    text_message dequeue(const recepient& = {}, const std::string& = {});

    bool dequeue(text_message& msg, unsigned wait_delay = 0, const recepient& = {}, const std::string& = {});

    void enqueue(const text_message&, const recepients& = {});
    const std::string& get_name() const { return m_name; }
  private:
    text_queue(const std::string& name, const std::shared_ptr<detail::text_queue>&);
    std::shared_ptr<detail::text_queue> impl_;
    friend class connection;
    const std::string m_name;
};

typedef std::shared_ptr<text_queue> text_queue_ptr;

} // namespace jms

#endif

