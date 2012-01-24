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
#include "astra_misc.h"
#include "jxtlib/JxtInterface.h"
#include "exceptions.h"

class TTripListFilter
{
  public:
    std::string airline;
    int flt_no;
    std::string suffix;
    std::string airp_dep;
    bool pr_cancel;
    bool pr_takeoff;
    TTripListFilter()
    {
      Clear();
    };
    void Clear()
    {
      airline.clear();
      flt_no=ASTRA::NoExists;
      suffix.clear();
      airp_dep.clear();
      pr_cancel=false;
      pr_takeoff=false;
    };
    void ToXML(xmlNodePtr node);
    void FromXML(xmlNodePtr node);
};

class TTripListView
{
  public:
    bool use_colors;
    TUserSettingType codes_fmt;
    TTripListView()
    {
      Clear();
    };
    void Clear()
    {
      use_colors=false;
      codes_fmt=ustCodeNative;
    };
    void ToXML(xmlNodePtr node);
    void FromXML(xmlNodePtr node);
};

class TTripListInfo
{
  public:
    BASIC::TDateTime date;
    int point_id;
    TTripListFilter filter;
    bool filter_from_xml;
    TTripListView view;
    bool view_from_xml;
    TTripListInfo()
    {
      Clear();
    };
    void Clear()
    {
      date=ASTRA::NoExists;
      point_id=ASTRA::NoExists;
      filter.Clear();
      filter_from_xml=false;
      view.Clear();
      view_from_xml=false;
    };
    void ToXML(xmlNodePtr node);
    void FromXML(xmlNodePtr node);
};

class TTripListSQLFilter
{
  public:
    TAccess access;
    std::map< TStage_Type, std::vector<TStage> > final_stages; //stage_type, вектор stage_id
    bool pr_cancel;
    bool pr_takeoff;
    std::pair<std::string, std::string> station; //название пульта, режим работы
    bool use_arrival_permit;
    virtual void set(void);
    virtual ~TTripListSQLFilter() {};
};

class TTripListSQLParams: public TTripListSQLFilter
{
  public:
    BASIC::TDateTime first_date, last_date;
    int flt_no;
    std::string suffix;
    int check_point_id;
};

class TTripInfoSQLParams: public TTripListSQLFilter
{
  public:
    int point_id;
    virtual void set(void);
};

int GetFltLoad( int point_id, const TTripInfo &fltInfo);
void readPaxLoad( int point_id, xmlNodePtr reqNode, xmlNodePtr resNode );
void viewCRSList( int point_id, xmlNodePtr dataNode );

class TripsInterface : public JxtInterface
{
private:
public:
  TripsInterface() : JxtInterface("","trips")
  {
     Handler *evHandle;
     evHandle=JxtHandler<TripsInterface>::CreateHandler(&TripsInterface::GetTripList);
     AddEvent("GetTripList",evHandle);
     AddEvent("GetAdvTripList",evHandle);
     evHandle=JxtHandler<TripsInterface>::CreateHandler(&TripsInterface::GetTripInfo);
     AddEvent("GetTripInfo",evHandle);
  };
  void GetTripList(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  void GetTripInfo(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode);
  virtual void Display(XMLRequestCtxt *ctxt, xmlNodePtr reqNode, xmlNodePtr resNode) {};

  void GetSegInfo(xmlNodePtr reqNode, xmlNodePtr resNode, xmlNodePtr dataNode);
  static void readOperFltHeader( const TTripInfo &info, xmlNodePtr node );
  static bool readTripHeader( int point_id, xmlNodePtr dataNode );
  static void readGates(int point_id, std::vector<std::string> &gates);
  static void readHalls( std::string airp_dep, std::string work_mode, xmlNodePtr dataNode);
};

bool Calc_overload_alarm( int point_id, const TTripInfo &fltInfo );
void Set_overload_alarm( int point_id, bool overload_alarm );
bool check_waitlist_alarm( int point_id );
bool check_brd_alarm( int point_id );
void Set_diffcomp_alarm( int point_id, bool diffcomp_alarm );

#endif /*_TRIPINFO_H_*/

