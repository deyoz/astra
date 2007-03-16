#ifndef _TRIPINFO_H_
#define _TRIPINFO_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include <map>
#include "basic.h"
#include "oralib.h"
#include "astra_utils.h"
#include "JxtInterface.h"

struct TVar {
  std::string name;
  otFieldType type;
  std::string value;
  TVar( std::string &aname, otFieldType atype, std::string &avalue ) {
    name = aname;
    type = atype;
    value = avalue;
  }
};

class TSQLParams {
  private:
    std::vector<TVar> vars;
  public:
    std::string sqlfrom;
    std::string sqlwhere;
    void addVariable( TVar &var );
    void addVariable( std::string aname, otFieldType atype, std::string avalue );
    void clearVariables( );
    void setVariables( TQuery &Qry );
};

class TSQL
{
private:
  std::map<std::string,TSQLParams> sqltrips;
  static TSQL *Instance();
  void createSQLTrips( );
public:
  TSQL();
  static void setSQLTripList( TQuery &Qry, TReqInfo &info );
  static void setSQLTripInfo( TQuery &Qry, TReqInfo &info );
};

std::string convertLastTrfer(std::string s);
void readPaxLoad( int point_id, xmlNodePtr reqNode, xmlNodePtr resNode );
void viewPNL( int point_id, xmlNodePtr dataNode );

class TTripInfo
{
  public:
    std::string airline,suffix,airp;
    int flt_no;
    BASIC::TDateTime scd_out,real_out;
};

std::string GetTripName( TTripInfo &info );


class TripsInterface : public JxtInterface
{
private:
public:
  TripsInterface() : JxtInterface("","trips")
  {
     Handler *evHandle;
     evHandle=JxtHandler<TripsInterface>::CreateHandler(&TripsInterface::GetTripList);
     AddEvent("GetTripList",evHandle);
     evHandle=JxtHandler<TripsInterface>::CreateHandler(&TripsInterface::GetTripInfo);
     AddEvent("GetTripInfo",evHandle);
  };
  void GetTripList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  static void TripsInterface::readTripHeader( int point_id, xmlNodePtr dataNode );
};

#endif /*_TRIPINFO_H_*/

