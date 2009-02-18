#ifndef _TRIPINFO_H_
#define _TRIPINFO_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include <map>
#include "basic.h"
#include "oralib.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "jxtlib/JxtInterface.h"

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
void viewCRSList( int point_id, xmlNodePtr dataNode );

class TTripInfo
{
  public:
    std::string airline,suffix,airp;
    int flt_no, pr_del, point_num, first_point;
    BASIC::TDateTime scd_out,real_out,real_out_local_date;
    bool pr_init;
    TTripInfo()
    {
      Clear();
    };
    TTripInfo( TQuery &Qry )
    {
      Init(Qry);
    };
    void Clear()
    {
      airline.clear();
      flt_no=0;
      suffix.clear();
      airp.clear();
      scd_out=ASTRA::NoExists;
      real_out=ASTRA::NoExists;
      real_out_local_date=ASTRA::NoExists; //GetTripName ��⠭�������� ���祭��
      pr_del = ASTRA::NoExists;
      point_num = ASTRA::NoExists;
      first_point = ASTRA::NoExists;
      pr_init = false;
    };
    void Init( TQuery &Qry )
    {
      airline=Qry.FieldAsString("airline");
      flt_no=Qry.FieldAsInteger("flt_no");
      suffix=Qry.FieldAsString("suffix");
      airp=Qry.FieldAsString("airp");
      scd_out=Qry.FieldAsDateTime("scd_out");
      if (Qry.GetFieldIndex("real_out")>=0)
        real_out = Qry.FieldAsDateTime("real_out");
      else
        real_out = ASTRA::NoExists;
      real_out_local_date=ASTRA::NoExists;
      if (Qry.GetFieldIndex("pr_del")>=0)
        pr_del = Qry.FieldAsInteger("pr_del");
      else
        pr_del = ASTRA::NoExists;
      if (Qry.GetFieldIndex("point_num")>=0)
        point_num = Qry.FieldAsInteger("point_num");
      else
        point_num = ASTRA::NoExists;
      if (Qry.GetFieldIndex("first_point")>=0)
        first_point = Qry.FieldAsInteger("first_point");
      else
        first_point = ASTRA::NoExists;
      pr_init = true;
    };
    bool Empty()
    {
      return !pr_init;
    };
};

std::string GetTripName( TTripInfo &info, bool showAirp=false, bool prList=false  );

//����ன�� ३�
enum TTripSetType { tsETLOnly=11, tsIgnoreTrferSet=12, tsMixedNorms=13 };
bool GetTripSets( TTripSetType setType, TTripInfo &info );


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

  static bool readTripHeader( int point_id, xmlNodePtr dataNode );
};

#endif /*_TRIPINFO_H_*/

