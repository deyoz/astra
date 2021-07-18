#ifndef _TRIPINFO_H_
#define _TRIPINFO_H_

#include <libxml/tree.h>
#include <string>
#include <vector>
#include <map>
#include "date_time.h"
#include "oralib.h"
#include "astra_consts.h"
#include "astra_utils.h"
#include "astra_misc.h"
#include "jxtlib/JxtInterface.h"
#include "exceptions.h"

using BASIC::date_time::TDateTime;

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
    TDateTime date;
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
    std::map< TStage_Type, std::vector<TStage> > final_stages; //stage_type, ����� stage_id
    bool pr_cancel;
    bool pr_takeoff;
    std::pair<std::string, std::string> station; //�������� ����, ०�� ࠡ���
    bool use_arrival_permit=false;
    virtual void set(void);
    virtual ~TTripListSQLFilter() {};
};

class TTripListSQLParams: public TTripListSQLFilter
{
  public:
    TDateTime first_date, last_date;
    int flt_no=ASTRA::NoExists;
    std::string suffix;
    int check_point_id=ASTRA::NoExists;
    bool includeScdIntoDateRange=false;
};

class TTripInfoSQLParams: public TTripListSQLFilter
{
  public:
    int point_id;
    virtual void set(void);
};

void setSQLTripList( DB::TQuery &Qry, const TTripListSQLFilter &filter );
std::optional<TStage> findFinalStage(const int point_id, const TStage_Type stage_type);

const std::string CREW_CLASS_ID=" ";
const std::string CREW_CLASS_VIEW=" ";
const int CREW_CLS_GRP_ID=1000000000;
const std::string CREW_CLS_GRP_VIEW=" ";
void readPaxLoad( int point_id, xmlNodePtr reqNode, xmlNodePtr resNode );
void viewCRSList( int point_id, const boost::optional<PaxId_t>& paxId, xmlNodePtr dataNode );
void viewPaxLoadSectionReport( int point_id, xmlNodePtr resNode );

bool SearchPaxByScanData(xmlNodePtr reqNode,
                         int &point_id,
                         int &reg_no,
                         int &pax_id);

bool SearchPaxByScanData(xmlNodePtr reqNode,
                         int &point_id,
                         int &reg_no,
                         int &pax_id,
                         bool &isBoardingPass);

namespace LIBRA {
//for LIBRA
struct CrsDataKey {
  std::string person;
  std::string airp_arv;
  std::string cls;
  CrsDataKey(const std::string& _person,
             const std::string& _airp_arv,
             const std::string& _cls ) {
   person = _person;
   airp_arv = _airp_arv;
   cls = _cls;
  }
  bool operator <(const CrsDataKey& key) const {
    if (person!=key.person)
        return person<key.person;
      if (airp_arv!=key.airp_arv)
        return airp_arv<key.airp_arv;
      return cls<key.cls;
  }
};
class CrsData: public std::map<CrsDataKey,int>
{
public:
  bool get( const PointId_t& point_id );
};
} // end namespace LIBRA

//end for LIBRA

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

  static void PectabsResponse(int point_id, xmlNodePtr reqNode, xmlNodePtr dataNode);
  void GetSegInfo(xmlNodePtr reqNode, xmlNodePtr resNode, xmlNodePtr dataNode);
  static void readOperFltHeader( const TTripInfo &info, xmlNodePtr node );
  static bool readTripHeader( int point_id, xmlNodePtr dataNode );
  static void readGates(int point_id, std::vector<std::string> &gates);
  static void readHalls( std::string airp_dep, std::string work_mode, xmlNodePtr dataNode);

  static TripsInterface* instance();
};

#endif /*_TRIPINFO_H_*/

