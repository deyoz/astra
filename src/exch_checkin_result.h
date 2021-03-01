#ifndef MQRABBIT_RESULTCHECKIN_H
#define MQRABBIT_RESULTCHECKIN_H

#include "astra_misc.h"
//#include "astra_service.h"


namespace MQRABBIT_TRANSPORT {

const std::string MQRABBIT_CHECK_IN_RESULT_OUT_TYPE = "MQRO";
const std::string MQRABBIT_FLIGHTS_RESULT_OUT_TYPE = "MQRF";
const std::string PARAM_NAME_ADDR = "ADDR";

struct MQRabbitParams {
  std::string addr;
  std::string queue;
  MQRabbitParams( const std::string &connect_str );
};

struct MQRabbitRequest {
  std::string Sender;
  std::vector<std::string> airps;
  std::vector<std::string> airlines;
  std::vector<int> flts;
  bool pr_reset;
  TDateTime lastRequestTime;
  MQRabbitRequest() {
    clear();
  }
  void clear() {
    lastRequestTime = ASTRA::NoExists;
    Sender.clear();
    airps.clear();
    airlines.clear();
    flts.clear();
    pr_reset = false;
  }
  virtual ~MQRabbitRequest(){}
};


class MQRSender {
public:
  virtual void send( const std::string& senderType,
                     const MQRabbitRequest &request,
                     std::map<std::string,std::string>& params )=0;
  void execute( const std::string& senderType );
  virtual ~MQRSender(){}
};

}

namespace EXCH_CHECKIN_RESULT {

struct Tids {
  int pax_tid;
  int grp_tid;
  Tids( ) {
    pax_tid = -1;
    grp_tid = -1;
  }
  Tids( int vpax_tid, int vgrp_tid ) {
    pax_tid = vpax_tid;
    grp_tid = vgrp_tid;
  }
};

void change_flight_props();

}

#endif // EXCH_CHECKIN_RESULT
