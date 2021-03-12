#ifndef _FILE_QUEUE_H_
#define _FILE_QUEUE_H_

#include <string>
#include <vector>
#include <map>
#include "date_time.h"
#include "oralib.h"
#include "astra_consts.h"

using BASIC::date_time::TDateTime;

const std::string PARAM_WORK_DIR = "WORKDIR";
const std::string PARAM_HTTP_ADDR = "HTTP_ADDR";
const std::string PARAM_CANON_NAME = "CANON_NAME";
const std::string PARAM_FILE_NAME = "FileName";

const std::string FILE_COLLECT_TYPE = "COLLECT";
const std::string FILE_HTTP_TYPEB_TYPE = "HTTP_TYPEB";
const std::string FILE_BAG_MESSAGE_TYPE = "BSM";

struct TFilterQueue {
  std::string receiver;
  std::string type;
  TDateTime last_time;
  int first_id;
  bool pr_first_order;
  int timeout_sec;
  void Init( const std::string &vreceiver,
                const std::string &vtype,
                const TDateTime &vlast_time,
                int vfirst_id,
                bool vpr_first_order,
                int vtimeout_sec ) {
    receiver = vreceiver;
    type = vtype;
    last_time = vlast_time;
    first_id = vfirst_id;
    pr_first_order = vpr_first_order;
    timeout_sec = vtimeout_sec;
  }
  TFilterQueue( const std::string &vreceiver,
                const std::string &vtype,
                const TDateTime &vlast_time,
                int vfirst_id,
                bool vpr_first_order,
                int vtimeout_sec ) {
    Init( vreceiver,
          vtype,
          vlast_time,
          vfirst_id,
          vpr_first_order,
          vtimeout_sec );
  }
  TFilterQueue( const TFilterQueue &vfilter ) {
    receiver = vfilter.receiver;
    type = vfilter.type;
    last_time = vfilter.last_time;
    first_id = vfilter.first_id;
    pr_first_order = vfilter.pr_first_order;
    timeout_sec = vfilter.timeout_sec;
  }
  TFilterQueue( const std::string &vreceiver,
                const std::string &vtype ) {
    Init( vreceiver, vtype, ASTRA::NoExists, ASTRA::NoExists, false, 0 );
  }
  TFilterQueue( const std::string &vreceiver,
                const std::string &vtype,
                int vfirst_id ) {
    Init( vreceiver, vtype, ASTRA::NoExists, vfirst_id, false, 0 );
  }
  TFilterQueue( const std::string &vreceiver,
                const std::string &vtype,
                const TDateTime &vlast_time ) {
    Init( vreceiver, vtype, vlast_time, ASTRA::NoExists, false, 0 );
  }
  TFilterQueue( const std::string &vreceiver,
                int vtimeout_sec ) {
    Init( vreceiver, std::string(""), ASTRA::NoExists, ASTRA::NoExists, true, vtimeout_sec );
  }
};

struct TQueueItem {
    int id;
    std::string receiver;
    std::string type;
    std::string status;
    TDateTime time;
    TDateTime wait_time;
    TDateTime put_time;
    std::string data;
    int in_order;
    std::map<std::string,std::string> params;
    TQueueItem() {
      id = ASTRA::NoExists;
      time = ASTRA::NoExists;
      wait_time = ASTRA::NoExists;
      put_time = ASTRA::NoExists;
      in_order = 0;
    }
};

class TFileQueue: public std::vector<TQueueItem> {
  private:
    char *pdata;
    bool pr_last_file;
  public:
    TFileQueue() {
      pdata = NULL;
      pr_last_file = true;
    }
    ~TFileQueue() {
      if ( pdata ) {
        free( pdata );
      }
    }
    static std::string getstatus( int id );
    static std::string gettype( int id );
    static std::string getFileData(int id);
    static std::pair<TDateTime, TDateTime> getwait_time(int id);
    static bool in_order( int id );
    static bool in_order( const std::string &type );
    static std::string getEncoding( const std::string &type,
                                    const std::string &point_addr,
                                    bool pr_send=true );
    static bool errorFile( int id, const std::string &msg );
    static bool sendFile( int id );
    static bool unsendFile( int id );
    static bool doneFile( int id );
    static bool deleteFile( int id );
    static int putFile( const std::string &receiver,
                        const std::string &sender,
                        const std::string &type,
                        const std::map<std::string,std::string> &params,
                        const std::string &file_data );
    static bool getparam_value( int id, const std::string &param_name, std::string &param_value );
    static void getparams( int id, std::map<std::string, std::string> &params );
    static void add_sets_params( const std::string &airp,
                                 const std::string &airline,
                                 const std::string &flt_no,
                                 const std::string &client_canon_name,
                                 const std::string &type,
                                 bool send,
                                 std::map<std::string,std::string> &params);
    void get( const TFilterQueue &filter, std::vector<TQueueItem> &items );
    void get( const TFilterQueue &filter ) {
      std::vector<TQueueItem> &items = *this;
      get( filter, items );
    }
    bool isLastFile() {
      return pr_last_file;
    }
};

int file_by_id(int argc,char **argv);

#endif /*_FILE_QUEUE_H_*/
