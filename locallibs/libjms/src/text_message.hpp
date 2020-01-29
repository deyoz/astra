#ifndef _TEXT_MESSAGE_HPP
#define _TEXT_MESSAGE_HPP

#include <list>
#include <string>

namespace jms
{

typedef std::string recepient;
typedef std::list<recepient> recepients;

struct message_properties
{
  int delay;
  int expiration;
  std::string correlation_id;
  recepient reply_to;
  message_properties() : delay(0), expiration(0) {}
  message_properties(int delay, int expiration, const std::string& correlation_id, const recepient& reply_to)
      : delay(delay), expiration(expiration), correlation_id(correlation_id), reply_to(reply_to)
  {}
};

struct text_message
{
  std::string text;
  message_properties properties;
};

}
#endif
