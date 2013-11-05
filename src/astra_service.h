#ifndef _ASTRA_SERVICE_H_
#define _ASTRA_SERVICE_H_

#include <libxml/tree.h>
#include <string>
#include <map>
#include "jxtlib/JxtInterface.h"
#include "astra_misc.h"

const std::string PARAM_TYPE = "type";
const std::string VALUE_TYPE_FILE = "FILE";
const std::string NS_PARAM_AIRP = "AIRP";
const std::string NS_PARAM_AIRLINE = "AIRLINE";
const std::string NS_PARAM_FLT_NO = "FLT_NO";
const std::string NS_PARAM_EVENT_TYPE = "EVENT_TYPE";
const std::string NS_PARAM_EVENT_ID1 = "EVENT_ID1";
const std::string NS_PARAM_EVENT_ID2 = "EVENT_ID2";
const std::string NS_PARAM_EVENT_ID3 = "EVENT_ID3";
const std::string FILE_UTG_TYPE = "UTG";

struct TFileData {
	std::string file_data;
	std::map<std::string,std::string> params;
};

typedef std::vector<TFileData> TFileDatas;


void getFileParams( const std::string client_canon_name, const std::string &type,
	                  int id, std::map<std::string, std::string> &fileparams, bool send );

void createSofiFileDATA( int receipt_id );
void createAODBFileDATA( int point_id );
void sync_aodb( void );
void sync_aodb( int point_id );
void sync_sppcek( void );
void sync_1ccek( void );
void sync_checkin_data( void );
void sync_checkin_data( int point_id );
void utg_prl(void);
void putUTG(int id, const std::string &basic_type, const TTripInfo &flt, const std::string &data);
void sync_fibs_data( );


class AstraServiceInterface : public JxtInterface
{
private:
public:
  AstraServiceInterface() : JxtInterface("","AstraService")
  {
     Handler *evHandle;
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::AstraTasksLogon);
     AddEvent("AstraTasksLogon",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::ThreadTaskReqData);
     AddEvent("ThreadTaskReqData",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::ThreadTaskResData);
     AddEvent("ThreadTaskResData",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::authorize);
     AddEvent("authorize",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::commitFileData);
     AddEvent("commitFileData",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::saveFileData);
     AddEvent("saveFileData",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::errorFileData);
     AddEvent("errorFileData",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::viewFileData);
     AddEvent("viewFileData",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::getFileParams);
     AddEvent("getFileParams",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::viewFileIds);
     AddEvent("viewFileIds",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::getAodbFiles);
     AddEvent("getAodbFiles",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::getAodbData);
     AddEvent("getAodbData",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::logFileData);
     AddEvent("Log",evHandle);
     evHandle=JxtHandler<AstraServiceInterface>::CreateHandler(&AstraServiceInterface::getTcpClientData);
     AddEvent("getTcpClientData",evHandle);
  };

  void AstraTasksLogon( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void ThreadTaskReqData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void ThreadTaskResData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void authorize( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void commitFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void saveFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void errorFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void createFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void viewFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void getFileParams( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void viewFileIds( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void getAodbFiles( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void getAodbData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void logFileData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  void getTcpClientData( XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode );
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
};

void CreateXMLStage( const TCkinClients &CkinClients, TStage stage_id, const TTripStage &stage,
                     xmlNodePtr node, const std::string &region );

void put_string_into_snapshot_points( int point_id, std::string file_type,
	                                    std::string point_addr, bool pr_old_record, std::string record );
void get_string_into_snapshot_points( int point_id, const std::string &file_type,
	                                    const std::string &point_addr, std::string &record );

#endif /*_ASTRA_SERVICE_H_*/

