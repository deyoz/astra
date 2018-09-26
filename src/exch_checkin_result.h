#ifndef MQRABBIT_RESULTCHECKIN_H
#define MQRABBIT_RESULTCHECKIN_H

#include "astra_misc.h"
#include "astra_service.h"


namespace MQRABBIT_TRANSPORT {

const std::string MQRABBIT_CHECK_IN_RESULT_OUT_TYPE = "MQRO";
const std::string MQRABBIT_FLIGHTS_RESULT_OUT_TYPE = "MQRF";
bool is_sync_exch_checkin_result_mqrabbit( const TTripInfo &tripInfo );
bool is_sync_exch_flights_result_mqrabbit( const TTripInfo &tripInfo );

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
