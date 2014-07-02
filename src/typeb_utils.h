#ifndef _TYPEB_UTILS_H_
#define _TYPEB_UTILS_H_

#include <sstream>
#include <string>
#include <tr1/memory>
#include "astra_locale.h"
#include "astra_misc.h"
#include "exceptions.h"
#include "xml_unit.h"
#include <typeinfo>

std::string typeid_name( const std::type_info& tinfo );

class localizedstream : public std::ostringstream
{
  private:
    std::string plang;
  public:
    localizedstream(std::string lang) : std::ostringstream(), plang(lang) {};
    std::string lang() {return plang;};
    std::string ElemIdToCodeNative(TElemType type, const std::string &elem)
    {
      return ::ElemIdToPrefferedElem(type, elem, efmtNameShort, plang);
    };
    std::string ElemIdToNameShort(TElemType type, const std::string &elem)
    {
      return ::ElemIdToPrefferedElem(type, elem, efmtNameShort, plang);
    };
                              
    std::string getLocaleText(const std::string &vlexema)
    {
      return AstraLocale::getLocaleText(vlexema, plang);
    };
};

namespace TypeB
{

const std::string endl = "\xa";

const std::string ERR_TAG_NAME = "ERROR";
const std::string DEFAULT_ERR = "?";
const std::string ERR_FIELD = "<" + ERR_TAG_NAME + ">" + DEFAULT_ERR + "</" + ERR_TAG_NAME + ">";

class TCreateOptions
{
  private:
    void init()
    {
      is_lat=false;
    };
  public:
    bool is_lat;
    TCreateOptions() {init();};
    virtual ~TCreateOptions() {};
    virtual void clear()
    {
      init();
    };
    virtual void fromXML(xmlNodePtr node)
    {
      clear();
      if (node==NULL) return;
      xmlNodePtr node2=node->children;
      is_lat=NodeAsIntegerFast( "pr_lat", node2, (int)is_lat )!=0;
    };
    virtual void fromDB(TQuery &Qry, TQuery &OptionsQry)
    {
      clear();
      is_lat=Qry.FieldAsInteger("pr_lat")!=0;

      const char* sql=
        "SELECT category, value FROM typeb_addr_options "
        "WHERE id=:id AND tlg_type=:tlg_type";
      if (strcmp(OptionsQry.SQLText.SQLText(),sql)!=0)
      {
        OptionsQry.Clear();
        OptionsQry.SQLText=sql;
        OptionsQry.DeclareVariable("id", otInteger);
        OptionsQry.DeclareVariable("tlg_type", otString);
      };

    };
    virtual localizedstream& logStr(localizedstream &s) const
    {
      s << s.getLocaleText("лат.") << ": "
        << (is_lat ? s.getLocaleText("да"):
                     s.getLocaleText("нет"));
      return s;
    };
    virtual localizedstream& extraStr(localizedstream &s) const
    {
      return s;
    };
    virtual std::string typeName() const
    {
      return "TCreateOptions";
    };
    virtual bool similar(const TCreateOptions &item) const
    {
      return is_lat==item.is_lat;
    };
    virtual bool equal(const TCreateOptions &item) const
    {
      return is_lat==item.is_lat;
    };
    virtual void copy(const TCreateOptions &item)
    {
      is_lat=item.is_lat;
    };
};

class TUnknownFmtOptions : public TCreateOptions
{
  private:
    void init()
    {
      extra.clear();
    };
  public:
    std::string extra;
    TUnknownFmtOptions() {init();};
    virtual ~TUnknownFmtOptions() {};
    virtual void clear()
    {
      TCreateOptions::clear();
      init();
    };
    virtual void fromXML(xmlNodePtr node)
    {
      TCreateOptions::fromXML(node);
      if (node==NULL) return;
      xmlNodePtr node2=node->children;
      extra=NodeAsStringFast( "extra", node2, extra.c_str() );
    };
    virtual localizedstream& logStr(localizedstream &s) const
    {
      TCreateOptions::logStr(s);
      s << ", "
        << s.getLocaleText("доп.") << ": "
        << extra;
      return s;
    };
    virtual localizedstream& extraStr(localizedstream &s) const
    {
      TCreateOptions::extraStr(s);
      s << extra << endl;
      return s;
    };
    virtual std::string typeName() const
    {
      return "TUnknownFmtOptions";
    };
    virtual bool similar(const TCreateOptions &item) const
    {
      if (!TCreateOptions::similar(item)) return false;
      try
      {
        dynamic_cast<const TUnknownFmtOptions&>(item);
        return true;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual bool equal(const TCreateOptions &item) const
    {
      if (!TCreateOptions::equal(item)) return false;
      try
      {
        dynamic_cast<const TUnknownFmtOptions&>(item);
        return true;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual void copy(const TCreateOptions &item)
    {
      TCreateOptions::copy(item);
      try
      {
        const TUnknownFmtOptions &opt = dynamic_cast<const TUnknownFmtOptions&>(item);
        extra=opt.extra;
      }
      catch(std::bad_cast) {};
    };
};

class TAirpTrferOptions : public TCreateOptions
{
  private:
    void init()
    {
      airp_trfer.clear();
    };
  public:
    std::string airp_trfer;
    TAirpTrferOptions() {init();};
    virtual ~TAirpTrferOptions() {};
    virtual void clear()
    {
      TCreateOptions::clear();
      init();
    };
    virtual void fromXML(xmlNodePtr node)
    {
      TCreateOptions::fromXML(node);
      if (node==NULL) return;
      xmlNodePtr node2=node->children;
      airp_trfer=NodeAsStringFast( "airp_arv", node2, airp_trfer.c_str() );
    };
    virtual void fromDB(TQuery &Qry, TQuery &OptionsQry)
    {
      TCreateOptions::fromDB(Qry, OptionsQry);
      airp_trfer=Qry.FieldAsString("airp_arv");
    };
    virtual localizedstream& logStr(localizedstream &s) const
    {
      TCreateOptions::logStr(s);
      s << ", "
        << s.getLocaleText("а/п") << ": "
        << s.ElemIdToCodeNative(etAirp, airp_trfer);
      return s;
    };
    virtual localizedstream& extraStr(localizedstream &s) const
    {
      TCreateOptions::extraStr(s);
      s << s.getLocaleText("а/п") << ": "
        << s.ElemIdToCodeNative(etAirp, airp_trfer)
        << endl;
      return s;
    };
    virtual std::string typeName() const
    {
      return "TAirpTrferOptions";
    };
    virtual bool similar(const TCreateOptions &item) const
    {
      if (!TCreateOptions::similar(item)) return false;
      try
      {
        const TAirpTrferOptions &opt = dynamic_cast<const TAirpTrferOptions&>(item);
        return airp_trfer.empty() || opt.airp_trfer.empty() || airp_trfer==opt.airp_trfer;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual bool equal(const TCreateOptions &item) const
    {
      if (!TCreateOptions::equal(item)) return false;
      try
      {
        const TAirpTrferOptions &opt = dynamic_cast<const TAirpTrferOptions&>(item);
        return airp_trfer==opt.airp_trfer;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual void copy(const TCreateOptions &item)
    {
      TCreateOptions::copy(item);
      try
      {
        const TAirpTrferOptions &opt = dynamic_cast<const TAirpTrferOptions&>(item);
        airp_trfer=opt.airp_trfer;
      }
      catch(std::bad_cast) {};
    };
};

class TCrsOptions : public TCreateOptions
{
  private:
    void init()
    {
      crs.clear();
    };
  public:
    std::string crs;
    TCrsOptions() {init();};
    virtual ~TCrsOptions() {};
    virtual void clear()
    {
      TCreateOptions::clear();
      init();
    };
    virtual void fromXML(xmlNodePtr node)
    {
      TCreateOptions::fromXML(node);
      if (node==NULL) return;
      xmlNodePtr node2=node->children;
      crs=NodeAsStringFast( "crs", node2, crs.c_str() );
    };
    virtual void fromDB(TQuery &Qry, TQuery &OptionsQry)
    {
      TCreateOptions::fromDB(Qry, OptionsQry);
      crs=Qry.FieldAsString("crs");
    };
    virtual localizedstream& logStr(localizedstream &s) const
    {
      TCreateOptions::logStr(s);
      if (!crs.empty())
        s << ", "
          << s.getLocaleText("центр") << ": "
          << crs;
      return s;
    };
    virtual localizedstream& extraStr(localizedstream &s) const
    {
      TCreateOptions::extraStr(s);
      if (!crs.empty())
        s << s.getLocaleText("центр") << ": "
          << crs
          << endl;
      return s;
    };
    virtual std::string typeName() const
    {
      return "TCrsOptions";
    };
    virtual bool similar(const TCreateOptions &item) const
    {
      if (!TCreateOptions::similar(item)) return false;
      try
      {
        const TCrsOptions &opt = dynamic_cast<const TCrsOptions&>(item);
        return crs==opt.crs;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual bool equal(const TCreateOptions &item) const
    {
      if (!TCreateOptions::equal(item)) return false;
      try
      {
        const TCrsOptions &opt = dynamic_cast<const TCrsOptions&>(item);
        return crs==opt.crs;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual void copy(const TCreateOptions &item)
    {
      TCreateOptions::copy(item);
      try
      {
        const TCrsOptions &opt = dynamic_cast<const TCrsOptions&>(item);
        crs=opt.crs;
      }
      catch(std::bad_cast) {};
    };
};

class TMarkInfoOptions : public TCrsOptions
{
  private:
    void init()
    {
      mark_info.clear();
      pr_mark_header=ASTRA::NoExists;
    };
  public:
    TSimpleMktFlight mark_info;
    int pr_mark_header;
    TMarkInfoOptions() {init();};
    virtual ~TMarkInfoOptions() {};
    virtual void clear()
    {
      TCrsOptions::clear();
      init();
    };
    virtual void fromXML(xmlNodePtr node)
    {
      TCrsOptions::fromXML(node);
      if (node==NULL) return;
      xmlNodePtr currNode = GetNode("CodeShare", node);
      if(currNode == NULL) return;
      currNode = currNode->children;
      mark_info.airline = NodeAsStringFast("airline_mark", currNode);
      mark_info.flt_no = NodeAsIntegerFast("flt_no_mark", currNode);
      mark_info.suffix = NodeAsStringFast("suffix_mark", currNode);
      pr_mark_header = (int)(NodeAsIntegerFast("pr_mark_header", currNode) != 0);
    };
    virtual void fromDB(TQuery &Qry, TQuery &OptionsQry)
    {
      TCrsOptions::fromDB(Qry, OptionsQry);
      if (Qry.FieldAsInteger("pr_mark_flt")!=0)
      {
        mark_info.airline = Qry.FieldAsString("airline");
        mark_info.flt_no = Qry.FieldIsNULL("flt_no")?ASTRA::NoExists:Qry.FieldAsInteger("flt_no");
        pr_mark_header = (int)(Qry.FieldAsInteger("pr_mark_header")!=0);
      };
    };
    virtual localizedstream& logStr(localizedstream &s) const
    {
      TCrsOptions::logStr(s);
      if (!mark_info.empty())
         s << ", "
           << s.getLocaleText("комм.рейс") << ": "
           << s.ElemIdToCodeNative(etAirline, mark_info.airline)
           << std::setw(3) << std::setfill('0') << mark_info.flt_no
           << s.ElemIdToCodeNative(etSuffix, mark_info.suffix);
      return s;
    };
    virtual localizedstream& extraStr(localizedstream &s) const
    {
      TCrsOptions::extraStr(s);
      if (!mark_info.empty())
        s << s.getLocaleText("комм.рейс") << ": "
          << s.ElemIdToCodeNative(etAirline, mark_info.airline)
          << std::setw(3) << std::setfill('0') << mark_info.flt_no
          << s.ElemIdToCodeNative(etSuffix, mark_info.suffix)
          << endl;
      if (pr_mark_header!=ASTRA::NoExists)
        s << s.getLocaleText("комм.заголовок") << ": "
          << (pr_mark_header!=0 ? s.getLocaleText("да"):
                                 s.getLocaleText("нет"))
          << endl;
      return s;
    };
    virtual std::string typeName() const
    {
      return "TMarkInfoOptions";
    };
    virtual bool similar(const TCreateOptions &item) const
    {
      if (!TCrsOptions::similar(item)) return false;
      try
      {
        const TMarkInfoOptions &opt = dynamic_cast<const TMarkInfoOptions&>(item);
        return pr_mark_header==opt.pr_mark_header &&
               (mark_info.airline.empty() ||
                opt.mark_info.airline.empty() ||
                mark_info.airline==opt.mark_info.airline) &&
               (mark_info.flt_no==ASTRA::NoExists ||
                opt.mark_info.flt_no==ASTRA::NoExists ||
                mark_info.flt_no==opt.mark_info.flt_no) &&
               (mark_info.suffix.empty() ||
                opt.mark_info.suffix.empty() ||
                mark_info.suffix==opt.mark_info.suffix);
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual bool equal(const TCreateOptions &item) const
    {
      if (!TCrsOptions::equal(item)) return false;
      try
      {
        const TMarkInfoOptions &opt = dynamic_cast<const TMarkInfoOptions&>(item);
        return pr_mark_header==opt.pr_mark_header &&
               mark_info.airline==opt.mark_info.airline &&
               mark_info.flt_no==opt.mark_info.flt_no &&
               mark_info.suffix==opt.mark_info.suffix;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual void copy(const TCreateOptions &item)
    {
      TCrsOptions::copy(item);
      try
      {
        const TMarkInfoOptions &opt = dynamic_cast<const TMarkInfoOptions&>(item);
        pr_mark_header=opt.pr_mark_header;
        mark_info=opt.mark_info;
      }
      catch(std::bad_cast) {};
    };
};

class TLDMOptions : public TCreateOptions
{
  private:
    void init()
    {
      version = "CEK";
      cabin_baggage=false;
      gender=false;
    };
  public:
    std::string version;
    bool cabin_baggage, gender;
    TLDMOptions() {init();};
    virtual ~TLDMOptions() {};
    virtual void clear()
    {
      TCreateOptions::clear();
      init();
    };
    virtual void fromXML(xmlNodePtr node)
    {
      TCreateOptions::fromXML(node);
      if (node==NULL) return;
      xmlNodePtr node2=node->children;
      version=NodeAsStringFast("version", node2, version.c_str());
      cabin_baggage=NodeAsIntegerFast("cabin_baggage", node2, (int)cabin_baggage) != 0;
      gender=NodeAsIntegerFast("gender", node2, (int)gender) != 0;
    };
    virtual void fromDB(TQuery &Qry, TQuery &OptionsQry)
    {
      TCreateOptions::fromDB(Qry, OptionsQry);
      OptionsQry.SetVariable("id", Qry.FieldAsInteger("id"));
      OptionsQry.SetVariable("tlg_type", Qry.FieldAsString("tlg_type"));
      OptionsQry.Execute();
      for(;!OptionsQry.Eof;OptionsQry.Next())
      {
        std::string cat=OptionsQry.FieldAsString("category");
        if (cat=="VERSION")
        {
          version=OptionsQry.FieldAsString("value");
          continue;
        };
        if (cat=="CABIN_BAGGAGE")
        {
          cabin_baggage=OptionsQry.FieldAsInteger("value")!=0;
          continue;
        };
        if (cat=="GENDER")
        {
          gender=OptionsQry.FieldAsInteger("value")!=0;
          continue;
        };
      };
    };
    virtual localizedstream& logStr(localizedstream &s) const
    {
      TCreateOptions::logStr(s);
      s << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LDM.VERSION") << ": "
        << s.ElemIdToNameShort(etTypeBOptionValue, "LDM+VERSION+"+version)
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LDM.CABIN_BAGGAGE") << ": "
        << (cabin_baggage ? s.getLocaleText("да"):
                            s.getLocaleText("нет"))
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LDM.GENDER") << ": "
        << (gender ? s.getLocaleText("да"):
                     s.getLocaleText("нет"));
      return s;
    };
    virtual localizedstream& extraStr(localizedstream &s) const
    {
      TCreateOptions::extraStr(s);
      s << s.getLocaleText("CAP.TYPEB_OPTIONS.LDM.VERSION") << ": "
        << s.ElemIdToNameShort(etTypeBOptionValue, "LDM+VERSION+"+version)
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LDM.CABIN_BAGGAGE") << ": "
        << (cabin_baggage ? s.getLocaleText("да"):
                            s.getLocaleText("нет"))
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LDM.GENDER") << ": "
        << (gender ? s.getLocaleText("да"):
                     s.getLocaleText("нет"))
        << endl;
      return s;
    };
    virtual std::string typeName() const
    {
      return "TLDMOptions";
    };
    virtual bool similar(const TCreateOptions &item) const
    {
      if (!TCreateOptions::similar(item)) return false;
      try
      {
        const TLDMOptions &opt = dynamic_cast<const TLDMOptions&>(item);
        return cabin_baggage==opt.cabin_baggage &&
            gender==opt.gender &&
            version == opt.version;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual bool equal(const TCreateOptions &item) const
    {
      if (!TCreateOptions::equal(item)) return false;
      try
      {
        const TLDMOptions &opt = dynamic_cast<const TLDMOptions&>(item);
        return cabin_baggage==opt.cabin_baggage &&
            gender==opt.gender &&
            version == opt.version;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual void copy(const TCreateOptions &item)
    {
      TCreateOptions::copy(item);
      try
      {
        const TLDMOptions &opt = dynamic_cast<const TLDMOptions&>(item);
        cabin_baggage=opt.cabin_baggage;
        gender=opt.gender;
        version = opt.version;
      }
      catch(std::bad_cast) {};
    };
};

class TPRLOptions : public TMarkInfoOptions
{
    private:
        void init()
        {
            create_point = "CLOSE_CKIN";
            pax_state = "CKIN";
        }
    public:
        std::string create_point;
        std::string pax_state;
        TPRLOptions() { init(); }
        virtual ~TPRLOptions() {};
        virtual void clear()
        {
            TMarkInfoOptions::clear();
            init();
        };
        virtual void fromXML(xmlNodePtr node)
        {
            TMarkInfoOptions::fromXML(node);
            if(node == NULL) return;
            xmlNodePtr node2=node->children;
            create_point = NodeAsStringFast("create_point", node2, create_point.c_str());
            pax_state = NodeAsStringFast("pax_state", node2, pax_state.c_str());
        }
        virtual void fromDB(TQuery &Qry, TQuery &OptionsQry)
        {
            TMarkInfoOptions::fromDB(Qry, OptionsQry);

            std::string basic_type;
            std::string tlg_type = Qry.FieldAsString("tlg_type");
            try
            {
                const TTypeBTypesRow& row = (TTypeBTypesRow&)(base_tables.get("typeb_types").get_row("code",tlg_type));
                basic_type=row.basic_type;
            }
            catch(EBaseTableError)
            {
                throw EXCEPTIONS::Exception("TPRLOptions::fromDB: unknown telegram type %s", tlg_type.c_str());
            };


            OptionsQry.SetVariable("id", Qry.FieldAsInteger("id"));
            OptionsQry.SetVariable("tlg_type", basic_type);
            OptionsQry.Execute();
            for(;!OptionsQry.Eof;OptionsQry.Next())
            {
                std::string cat=OptionsQry.FieldAsString("category");
                if (cat=="CREATE_POINT")
                {
                    create_point = OptionsQry.FieldAsString("value");
                    continue;
                };
                if (cat=="PAX_STATE")
                {
                    pax_state = OptionsQry.FieldAsString("value");
                    continue;
                };
            }
        }
        virtual localizedstream& logStr(localizedstream &s) const
        {
            TMarkInfoOptions::logStr(s);
            s
                << ", "
                << s.getLocaleText("CAP.TYPEB_OPTIONS.PRL.CREATE_POINT") << ": "
                << s.ElemIdToNameShort(etTypeBOptionValue, "PRL+CREATE_POINT+"+create_point)
                << ", "
                << s.getLocaleText("CAP.TYPEB_OPTIONS.PRL.PAX_STATE") << ": "
                << s.ElemIdToNameShort(etTypeBOptionValue, "PRL+PAX_STATE+"+pax_state);
            return s;
        }
        virtual localizedstream& extraStr(localizedstream &s) const
        {
            TMarkInfoOptions::extraStr(s);
            s
                << s.getLocaleText("CAP.TYPEB_OPTIONS.PRL.CREATE_POINT") << ": "
                << s.ElemIdToNameShort(etTypeBOptionValue, "PRL+CREATE_POINT+"+create_point)
                << endl
                << s.getLocaleText("CAP.TYPEB_OPTIONS.PRL.PAX_STATE") << ": "
                << s.ElemIdToNameShort(etTypeBOptionValue, "PRL+PAX_STATE+"+pax_state)
                << endl;
            return s;
        };
        virtual std::string typeName() const
        {
            return "TPRLOptions";
        };
        virtual bool similar(const TCreateOptions &item) const
        {
            if (!TMarkInfoOptions::similar(item)) return false;
            try
            {
                const TPRLOptions &opt = dynamic_cast<const TPRLOptions&>(item);
                return
                    create_point == opt.create_point and
                    pax_state == opt.pax_state;
            }
            catch(std::bad_cast)
            {
                return false;
            };
        }
        virtual bool equal(const TCreateOptions &item) const
        {
            if (!TMarkInfoOptions::equal(item)) return false;
            try
            {
                const TPRLOptions &opt = dynamic_cast<const TPRLOptions&>(item);
                return
                    create_point == opt.create_point and
                    pax_state == opt.pax_state;
            }
            catch(std::bad_cast)
            {
                return false;
            };
        };
        virtual void copy(const TCreateOptions &item)
        {
            TMarkInfoOptions::copy(item);
            try
            {
                const TPRLOptions &opt = dynamic_cast<const TPRLOptions&>(item);
                create_point = opt.create_point;
                pax_state = opt.pax_state;
            }
            catch(std::bad_cast) {};
        };
};

class TLCIOptions : public TCreateOptions
{
  private:
    void init()
    {
      equipment=true;
      weight_avail="PU";
      seating=true;
      weight_mode=true;
      seat_restrict="CZ";
      pas_totals = true;
      bag_totals = true;
      pas_distrib = true;
      seat_plan = true;
    };
  public:
    bool equipment, seating, weight_mode;
    std::string weight_avail, seat_restrict;
    bool pas_totals, bag_totals, pas_distrib, seat_plan;
    TLCIOptions() {init();};
    virtual ~TLCIOptions() {};
    virtual void clear()
    {
      TCreateOptions::clear();
      init();
    };
    virtual void fromXML(xmlNodePtr node)
    {
      TCreateOptions::fromXML(node);
      if (node==NULL) return;
      xmlNodePtr node2=node->children;
      equipment=NodeAsIntegerFast("equipment", node2, (int)equipment) != 0;
      weight_avail=NodeAsStringFast("weight_avail", node2, weight_avail.c_str());
      seating=NodeAsIntegerFast("seating", node2, (int)seating) != 0;
      weight_mode=NodeAsIntegerFast("weight_mode", node2, (int)weight_mode) != 0;
      seat_restrict=NodeAsStringFast("seat_restrict", node2, seat_restrict.c_str());
      pas_totals=NodeAsIntegerFast("pas_totals", node2, (int)pas_totals) != 0;
      bag_totals=NodeAsIntegerFast("bag_totals", node2, (int)bag_totals) != 0;
      pas_distrib=NodeAsIntegerFast("pas_distrib", node2, (int)pas_distrib) != 0;
      seat_plan=NodeAsIntegerFast("seat_plan", node2, (int)seat_plan) != 0;
    };
    virtual void fromDB(TQuery &Qry, TQuery &OptionsQry)
    {
      TCreateOptions::fromDB(Qry, OptionsQry);
      OptionsQry.SetVariable("id", Qry.FieldAsInteger("id"));
      OptionsQry.SetVariable("tlg_type", Qry.FieldAsString("tlg_type"));
      OptionsQry.Execute();
      for(;!OptionsQry.Eof;OptionsQry.Next())
      {
        std::string cat=OptionsQry.FieldAsString("category");
        if (cat=="EQUIPMENT")
        {
          equipment=OptionsQry.FieldAsInteger("value")!=0;
          continue;
        };
        if (cat=="WEIGHT_AVAIL")
        {
          weight_avail=OptionsQry.FieldAsString("value");
          continue;
        };
        if (cat=="SEATING")
        {
          seating=OptionsQry.FieldAsInteger("value")!=0;
          continue;
        };
        if (cat=="WEIGHT_MODE")
        {
          weight_mode=OptionsQry.FieldAsInteger("value")!=0;
          continue;
        };
        if (cat=="SEAT_RESTRICT")
        {
          seat_restrict=OptionsQry.FieldAsString("value");
          continue;
        };
        if (cat=="PAS_TOTALS")
        {
          pas_totals=OptionsQry.FieldAsInteger("value")!=0;
          continue;
        };
        if (cat=="BAG_TOTALS")
        {
          bag_totals=OptionsQry.FieldAsInteger("value")!=0;
          continue;
        };
        if (cat=="PAS_DISTRIB")
        {
          pas_distrib=OptionsQry.FieldAsInteger("value")!=0;
          continue;
        };
        if (cat=="SEAT_PLAN")
        {
          seat_plan=OptionsQry.FieldAsInteger("value")!=0;
          continue;
        };
      };
    };
    virtual localizedstream& logStr(localizedstream &s) const
    {
      TCreateOptions::logStr(s);
      s << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.EQUIPMENT") << ": "
        << (equipment ? s.getLocaleText("да"):
                        s.getLocaleText("нет"))
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.WEIGHT_AVAIL") << ": "
        << s.ElemIdToNameShort(etTypeBOptionValue, "LCI+WEIGHT_AVAIL+"+weight_avail)
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.SEATING") << ": "
        << (seating ? s.getLocaleText("да"):
                      s.getLocaleText("нет"))
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.WEIGHT_MODE") << ": "
        << (weight_mode ? s.getLocaleText("да"):
                          s.getLocaleText("нет"))
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.SEAT_RESTRICT") << ": "
        << s.ElemIdToNameShort(etTypeBOptionValue, "LCI+SEAT_RESTRICT+"+seat_restrict)
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.PAS_TOTALS") << ": "
        << (pas_totals ? s.getLocaleText("да"):
                         s.getLocaleText("нет"))
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.BAG_TOTALS") << ": "
        << (bag_totals ? s.getLocaleText("да"):
                         s.getLocaleText("нет"))
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.PAS_DISTRIB") << ": "
        << (pas_distrib ? s.getLocaleText("да"):
                          s.getLocaleText("нет"))
        << ", "
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.SEAT_PLAN") << ": "
        << (seat_plan ? s.getLocaleText("да"):
                        s.getLocaleText("нет"));
      return s;
    };
    virtual localizedstream& extraStr(localizedstream &s) const
    {
      TCreateOptions::extraStr(s);
      s << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.EQUIPMENT") << ": "
        << (equipment ? s.getLocaleText("да"):
                        s.getLocaleText("нет"))
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.WEIGHT_AVAIL") << ": "
        << s.ElemIdToNameShort(etTypeBOptionValue, "LCI+WEIGHT_AVAIL+"+weight_avail)
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.SEATING") << ": "
        << (seating ? s.getLocaleText("да"):
                      s.getLocaleText("нет"))
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.WEIGHT_MODE") << ": "
        << (weight_mode ? s.getLocaleText("да"):
                          s.getLocaleText("нет"))
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.SEAT_RESTRICT") << ": "
        << s.ElemIdToNameShort(etTypeBOptionValue, "LCI+SEAT_RESTRICT+"+seat_restrict)
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.PAS_TOTALS") << ": "
        << (pas_totals ? s.getLocaleText("да"):
                         s.getLocaleText("нет"))
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.BAG_TOTALS") << ": "
        << (bag_totals ? s.getLocaleText("да"):
                         s.getLocaleText("нет"))
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.PAS_DISTRIB") << ": "
        << (pas_distrib ? s.getLocaleText("да"):
                          s.getLocaleText("нет"))
        << endl
        << s.getLocaleText("CAP.TYPEB_OPTIONS.LCI.SEAT_PLAN") << ": "
        << (seat_plan ? s.getLocaleText("да"):
                        s.getLocaleText("нет"))
        << endl;
      return s;
    };
    virtual std::string typeName() const
    {
      return "TLCIOptions";
    };
    virtual bool similar(const TCreateOptions &item) const
    {
      if (!TCreateOptions::similar(item)) return false;
      try
      {
        const TLCIOptions &opt = dynamic_cast<const TLCIOptions&>(item);
        return equipment==opt.equipment &&
               weight_avail==opt.weight_avail &&
               seating==opt.seating &&
               weight_mode==opt.weight_mode &&
               seat_restrict==opt.seat_restrict &&
               pas_totals==opt.pas_totals &&
               bag_totals==opt.bag_totals &&
               pas_distrib==opt.pas_distrib &&
               seat_plan==opt.seat_plan;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual bool equal(const TCreateOptions &item) const
    {
      if (!TCreateOptions::equal(item)) return false;
      try
      {
        const TLCIOptions &opt = dynamic_cast<const TLCIOptions&>(item);
        return equipment==opt.equipment &&
               weight_avail==opt.weight_avail &&
               seating==opt.seating &&
               weight_mode==opt.weight_mode &&
               seat_restrict==opt.seat_restrict &&
               pas_totals==opt.pas_totals &&
               bag_totals==opt.bag_totals &&
               pas_distrib==opt.pas_distrib &&
               seat_plan==opt.seat_plan;
      }
      catch(std::bad_cast)
      {
        return false;
      };
    };
    virtual void copy(const TCreateOptions &item)
    {
      TCreateOptions::copy(item);
      try
      {
        const TLCIOptions &opt = dynamic_cast<const TLCIOptions&>(item);
        equipment=opt.equipment;
        weight_avail=opt.weight_avail;
        seating=opt.seating;
        weight_mode=opt.weight_mode;
        seat_restrict=opt.seat_restrict;
        pas_totals=opt.pas_totals;
        bag_totals=opt.bag_totals;
        pas_distrib=opt.pas_distrib;
        seat_plan=opt.seat_plan;
      }
      catch(std::bad_cast) {};
    };
};

std::tr1::shared_ptr<TCreateOptions> make_options(const std::string &tlg_type);

class TOptionsInfo
{
  private:
    std::string tlg_type;
    std::tr1::shared_ptr<TCreateOptions> options;
    void init()
    {
      tlg_type.clear();
      options=make_options(tlg_type);
    };
  protected:
    bool operator == (const TOptionsInfo &info) const
    {
      return tlg_type==info.tlg_type &&
             get_options().equal(info.get_options());
    };
  public:
    const std::string& get_tlg_type() const
    {
      return tlg_type;
    };
    const TCreateOptions& get_options() const
    {
      return *options.get();
    };

    TOptionsInfo() {init();};
    TOptionsInfo(const std::string &p_tlg_type)
    {
      tlg_type=p_tlg_type;
      options=make_options(tlg_type);
    };
    virtual ~TOptionsInfo() {};

    virtual void clear()
    {
      init();
    };

    virtual void fromXML(xmlNodePtr node)
    {
      clear();
      if (node==NULL) return;
      xmlNodePtr node2=node->children;
      tlg_type=NodeAsStringFast( "tlg_type", node2);
      options=make_options(tlg_type);
      options.get()->fromXML(node);
    };

    virtual void fromDB(TQuery &Qry, TQuery &OptionsQry)
    {
      clear();
      tlg_type=Qry.FieldAsString("tlg_type");
      options=make_options(tlg_type);
      options.get()->fromDB(Qry, OptionsQry);
    };

    virtual std::string typeName() const
    {
      return "TOptionsInfo";
    };

    template <typename T>
    T* optionsAs() const
    {
      T* result = dynamic_cast<T*>(options.get());
      if (result==NULL)
        throw EXCEPTIONS::Exception("TypeB::%s.optionsAs: invalid cast from TypeB::%s to %s",
                                    typeName().c_str(),
                                    options.get()->typeName().c_str(),
                                    typeid_name(typeid(T)).c_str());
      return result;
    };

    template <typename T>
    bool optionsIs() const
    {
      return dynamic_cast<T*>(options.get())!=NULL;
    };

    void copy(const TOptionsInfo &info)
    {
      tlg_type=info.tlg_type;
      options=make_options(tlg_type);
      options.get()->copy(*(info.options.get()));
    };
};

struct TCreatePoint {
    TStage stage_id;
    int time_offset;
    TCreatePoint() { clear(); }
    TCreatePoint(const std::string &params) { paramsFromString(params); }
    TCreatePoint(TStage vstage_id, int vtime_offset): stage_id(vstage_id), time_offset(vtime_offset) {}
    void clear() { stage_id = sNoActive; time_offset = 0; };
    bool exists(int typeb_addrs_id, const std::string &tlg_type) const;
    bool operator < (const TCreatePoint &cp) const
    {
        if(stage_id != cp.stage_id)
            return stage_id < cp.stage_id;
        return time_offset < cp.time_offset;
    }
    bool operator == (const TCreatePoint &cp) const
    {
        return
            time_offset == cp.time_offset and
            stage_id == cp.stage_id;
    }
    void paramsFromString(const std::string &params);
    std::string paramsToString() const;
};

class TCreateInfo : public TOptionsInfo
{
  private:
    void init()
    {
      point_id=ASTRA::NoExists;
      addrs.clear();
      create_point.clear();
    };
  public:
    int point_id;
    std::set<std::string> addrs;
    TCreatePoint create_point;

    TCreateInfo() {init();};
    TCreateInfo(const std::string &tlg_type, const TCreatePoint &vcreate_point):TOptionsInfo(tlg_type)
    {
        init();
    };
    virtual ~TCreateInfo() {};

    virtual void clear()
    {
      TOptionsInfo::clear();
      init();
    };

    void set_addrs(const std::string &s)
    {
      addrs.clear();
      std::string::const_iterator iStart=s.end();
      for(std::string::const_iterator i=s.begin();;++i)
      {
        if (i==s.end() || (*i>=0x00 && *i<=0x20))
        {
          //это пробел или конец строки
          if (iStart!=s.end())
          {
            addrs.insert(std::string(iStart,i));
            iStart=s.end();
          };
        }
        else
        {
          //это символ
          if (iStart==s.end()) iStart=i;
        };
        if (i==s.end()) break;
      };
    };

    std::string get_addrs() const
    {
      std::ostringstream result;
      for(std::set<std::string>::const_iterator i=addrs.begin();i!=addrs.end();i++)
        result << *i << " ";
      return result.str();
    };

    virtual void fromXML(xmlNodePtr node)
    {
      TOptionsInfo::fromXML(node);
      if (node==NULL) return;
      xmlNodePtr node2=node->children;
      if (!NodeIsNULLFast( "point_id", node2 ))
      {
        point_id = NodeAsIntegerFast( "point_id", node2 );
        if (point_id==-1) point_id=ASTRA::NoExists;
      };
      set_addrs(NodeAsStringFast( "addrs", node2));
    };

    virtual void fromDB(TQuery &Qry, TQuery &OptionsQry)
    {
      TOptionsInfo::fromDB(Qry, OptionsQry);
      set_addrs(Qry.FieldAsString("addr"));
    };

    localizedstream& logStr(localizedstream &s) const
    {
      get_options().logStr(s) << ", "
                              << s.getLocaleText("адреса") << ": "
                              << get_addrs();
      return s;
    };

    virtual std::string typeName() const
    {
      return "TCreateInfo";
    };

    bool canMerge(const TCreateInfo &info) const
    {
      return (*this == info &&
              point_id == info.point_id);
    };
    void dump() const;

    void copy(const TCreateInfo &info)
    {
      TOptionsInfo::copy(info);
      point_id=info.point_id;
      create_point = info.create_point;
      addrs=info.addrs;
    };
};

struct TOriginatorInfo
{
  int id;
  std::string addr;
  std::string double_sign;
  std::string descr;
  TOriginatorInfo() { clear(); };
  void clear()
  {
    id=ASTRA::NoExists;
    addr.clear();
    double_sign.clear();
    descr.clear();
  };
  TOriginatorInfo& fromDB(TQuery &Qry);
  std::string originSection(BASIC::TDateTime time_create, const std::string &endline) const;
};

struct TDraftPart {
    std::string addr, origin, heading, ending, body;
};

struct TTypeBOutErrMsg {
    int err_pos;
    int err_len;
    std::map<std::string, std::string> msg;
    TTypeBOutErrMsg():
        err_pos(0),
        err_len(0)
    {}
};

struct err_lst_cmp {
    bool operator()(const std::pair<int, TTypeBOutErrMsg *> &lhs, const std::pair<int, TTypeBOutErrMsg *> &rhs) const
    {
        return lhs.second->err_pos < rhs.second->err_pos;
    }
};

typedef std::set<std::pair<int, TTypeBOutErrMsg *>, err_lst_cmp> t_sorted_err_lst;

enum TTlgInOut {tioOut, tioIn};

struct TErrLst:std::map<int, TTypeBOutErrMsg> {
    private:
        TTlgInOut tio;
        int tlg_id;
        int num;
        bool common_lst; // общий список ошибок для всех частей
        t_sorted_err_lst sorted_err_lst;
        int err_no;
        int endl_offset;
        xmlNodePtr errLst;
        int fix_endl(const std::string &val, size_t pos = std::string::npos);
        int fix_err_len(const std::string &val, size_t curr_pos, int err_len);
    public:
        int pos;
        void dump();
        void toDB(int tlg_id);
        void fix(std::vector<TDraftPart> &parts);
        void fetch_err(std::set<int> &txt_errs, std::string body);

        void pack(TypeB::TDraftPart &part, bool heading_visible, bool ending_visible);
        void pack(std::string &val, bool visible = true);

        void unpack(TypeB::TDraftPart &draft, bool heading_visible, bool ending_visible);
        void unpack(std::string &val, bool visible = true);

        void fromDB(int tlg_id, int num);
        void toXML(xmlNodePtr node, const TypeB::TDraftPart &part, bool is_first_part, bool is_last_part, const std::string &lang);
        void toXML(const std::string &val, const std::string &lang, bool visible = true);

        std::string add_err(std::string err, const AstraLocale::LexemaData &ld);
        std::string add_err(std::string err, std::string val);
        std::string add_err(std::string err, const char *format, ...);

        TErrLst(TTlgInOut atio):
            tio(atio),
            tlg_id(ASTRA::NoExists),
            num(ASTRA::NoExists),
            common_lst(false),
            err_no(0),
            endl_offset(0),
            pos(0)
    {};
        TErrLst(TTlgInOut atio, int tlg_id):
            tio(atio),
            tlg_id(ASTRA::NoExists),
            num(ASTRA::NoExists),
            common_lst(false),
            err_no(0),
            endl_offset(0),
            pos(0)
    { fromDB(tlg_id, ASTRA::NoExists); };
};

class TDetailCreateInfo : public TOptionsInfo
{
  public:
    TCreatePoint create_point;
    //адреса получателей
    std::string addrs;
    //адрес отправителя
    TOriginatorInfo originator;
    //время создания
    BASIC::TDateTime time_create;
    //рейс
    int point_id;
    std::string airline;
    int flt_no;
    std::string suffix;
    std::string airp_dep;
    std::string airp_arv;
    BASIC::TDateTime scd_utc;
    BASIC::TDateTime est_utc;
    BASIC::TDateTime scd_local;
    BASIC::TDateTime act_local;
    int scd_local_day;
    std::string bort;
    std::string craft;
    // кодировка салона
    bool pr_lat_seat;
    //вспомогательные чтобы вытаскивать маршрут
    int first_point;
    int point_num;
    bool pr_tranzit;
    //разные настройки
    bool vcompleted;
    bool manual_creation;
    TElemFmt elem_fmt;
    std::string lang;
    // список ошибок телеграммы
    TErrLst err_lst;

    std::string TlgElemIdToElem(TElemType type, int id, TElemFmt fmt = efmtUnknown);
    std::string TlgElemIdToElem(TElemType type, std::string id, TElemFmt fmt = efmtUnknown);
    bool operator == (const TMktFlight &s) const;

    bool is_lat();
    std::string airline_view();
    std::string flt_no_view();
    std::string suffix_view();
    std::string airp_dep_view();
    std::string airp_trfer_view();
    std::string airp_arv_view();
    std::string flight_view();
    std::string scd_local_view();
    std::string airline_mark() const;

    TDetailCreateInfo(): err_lst(tioOut)
    {
        time_create = ASTRA::NoExists;
        point_id = ASTRA::NoExists;
        flt_no = ASTRA::NoExists;
        scd_utc = ASTRA::NoExists;
        est_utc = ASTRA::NoExists;
        scd_local = ASTRA::NoExists;
        act_local = ASTRA::NoExists;
        scd_local_day = ASTRA::NoExists;
        pr_lat_seat=false;
        first_point = ASTRA::NoExists;
        point_num = ASTRA::NoExists;
        pr_tranzit = false;
        vcompleted = false;
        manual_creation = false;
        elem_fmt = efmtUnknown;
    };
    virtual ~TDetailCreateInfo() {};

    virtual std::string typeName() const
    {
      return "TDetailCreateInfo";
    };
};

std::string TlgElemIdToElem(TElemType type, int id, TElemFmt fmt, std::string lang);
std::string TlgElemIdToElem(TElemType type, std::string id, TElemFmt fmt, std::string lang);

std::string fetch_addr(std::string &addr, TDetailCreateInfo *info = NULL);
std::string format_addr_line(std::string vaddrs, TDetailCreateInfo *info = NULL);

TOriginatorInfo getOriginator(const std::string &airline,
                              const std::string &airp_dep,
                              const std::string &tlg_type,
                              const BASIC::TDateTime &time_create,
                              bool with_exception);

class TSendInfo
{
  public:
    std::string tlg_type,airline,airp_dep,airp_arv;
    int flt_no,point_id,first_point,point_num;
    bool pr_tranzit;
    TCreatePoint create_point;

    TSendInfo() {clear();};
    TSendInfo(const std::string &p_tlg_type, const TAdvTripInfo &fltInfo, const TCreatePoint &vcreate_point)
    {
      clear();
      tlg_type=p_tlg_type;
      airline=fltInfo.airline;
      flt_no=fltInfo.flt_no;
      airp_dep=fltInfo.airp;
      point_id=fltInfo.point_id;
      point_num=fltInfo.point_num;
      first_point=fltInfo.first_point;
      pr_tranzit=fltInfo.pr_tranzit;
      create_point = vcreate_point;
    };
    TSendInfo(const TDetailCreateInfo &info)
    {
      clear();
      tlg_type=info.get_tlg_type();
      airline=info.airline;
      flt_no=info.flt_no;
      airp_dep=info.airp_dep;
      airp_arv=info.airp_arv;
      point_id=info.point_id;
      first_point=info.first_point;
      point_num=info.point_num;
      pr_tranzit=info.pr_tranzit;
    };
    void clear()
    {
      tlg_type.clear();
      airline.clear();
      airp_dep.clear();
      airp_arv.clear();
      flt_no=ASTRA::NoExists;
      point_id=ASTRA::NoExists;
      first_point=ASTRA::NoExists;
      point_num=ASTRA::NoExists;
      pr_tranzit=false;
      create_point.clear();
    };
    bool isSend() const;
    void getCreateInfo(const std::vector<TSimpleMktFlight> &mktFlights,
                       bool onlyOneFlight,
                       std::vector<TCreateInfo> &info) const;
    void getCreatePoints(const std::vector<TSimpleMktFlight> &mktFlights,
                         std::set<TCreatePoint> &info) const;
};

class TAddrInfo : public TOptionsInfo
{
  private:
    void init()
    {
      sendInfo.clear();
    };
  public:
    TSendInfo sendInfo;
    TAddrInfo() {init();};
    TAddrInfo(const TSendInfo &info) : TOptionsInfo(info.tlg_type)
    {
      sendInfo=info;
    };
    virtual ~TAddrInfo() {};
    virtual void clear()
    {
      TOptionsInfo::clear();
      init();
    };
    virtual void fromXML(xmlNodePtr node)
    {
      TOptionsInfo::fromXML(node);
      sendInfo.tlg_type=get_tlg_type();
      if (node==NULL) return;
      xmlNodePtr node2=node->children;
      if (!NodeIsNULLFast( "point_id", node2 ))
      {
        sendInfo.point_id = NodeAsIntegerFast( "point_id", node2 );
        if (sendInfo.point_id==-1) sendInfo.point_id=ASTRA::NoExists;
      };
    };
    std::string getAddrs() const;
    virtual std::string typeName() const
    {
      return "TAddrInfo";
    };
};

class TCreator
{
  private:
    std::set<std::string> tlg_types;
    TAdvTripInfo flt;

    std::set<std::string> p_airps;
    bool airps_init;
    std::vector<std::string> p_crs;
    bool crs_init;
    std::vector<TSimpleMktFlight> p_mkt_flights;
    bool mkt_flights_init;

    const std::set<std::string>& airps();
    const std::vector<std::string>& crs();
    const std::vector<TSimpleMktFlight>& mkt_flights();
    
    TCreatePoint create_point;
  public:
    TCreator(const TAdvTripInfo &fltInfo):flt(fltInfo)
    {
      airps_init=false;
      crs_init=false;
      mkt_flights_init=false;
    };
    TCreator(int point_id, const TCreatePoint &vcreate_point);
    virtual ~TCreator() {};

    TCreator& operator << (const std::string &tlg_type)
    {
      tlg_types.insert(tlg_type);
      return *this;
    };

    void getInfo(std::vector<TCreateInfo> &info);

    virtual bool validInfo(const TCreateInfo &info) const {return true;};

};

class TCloseCheckInCreator : public TCreator
{
  public:
    TCloseCheckInCreator(int point_id) : TCreator(point_id, TCreatePoint(sCloseCheckIn, 0))
    {
      *this << "COM"
            << "COM2"
            << "PRL";
    };

    virtual bool validInfo(const TCreateInfo &info) const {
        if (!TCreator::validInfo(info)) return false;

        if (info.optionsIs<TPRLOptions>())
        {
          if (info.optionsAs<TPRLOptions>()->create_point!="CLOSE_CKIN") return false;
        };    

        return true;
    };
};

class TCloseBoardingCreator : public TCreator
{
  public:
    TCloseBoardingCreator(int point_id) : TCreator(point_id, TCreatePoint(sCloseBoarding, 0))
    {
      *this << "COM"
            << "COM2"
            << "PRL";
    };
    virtual bool validInfo(const TCreateInfo &info) const {
        if (!TCreator::validInfo(info)) return false;

        if (info.optionsIs<TPRLOptions>())
        {
          if (info.optionsAs<TPRLOptions>()->create_point!="CLOSE_BRD") return false;
        };    

        return true;
    };
};

class TTakeoffCreator : public TCreator
{
  public:
    TTakeoffCreator(int point_id) : TCreator(point_id, TCreatePoint(sTakeoff, 0))
    {
      *this << "PTM"
            << "PTMN"
            << "BTM"
            << "TPM"
            << "PSM"
            << "PFS"
            << "PFSN"
            << "FTL"
            << "PRL"
            << "PIM"
            << "SOM"
        //    << "ETL" формируем по прилету в конечные пункт если не было интерактива с СЭБ
            << "ETLD"
            << "LDM"
            << "CPM";
    };
    virtual bool validInfo(const TCreateInfo &info) const {
        if (!TCreator::validInfo(info)) return false;

        if (info.optionsIs<TPRLOptions>())
        {
          if (info.optionsAs<TPRLOptions>()->create_point!="TAKEOFF") return false;
        };    

        return true;
    };
};

class TMVTACreator : public TCreator
{
  public:
    TMVTACreator(int point_id) : TCreator(point_id, TCreatePoint())
    {
      *this << "MVTA";
    };
};

class TMVTBCreator : public TCreator
{
  public:
    TMVTBCreator(int point_id) : TCreator(point_id, TCreatePoint())
    {
      *this << "MVTB";
    };
};

class TMVTCCreator : public TCreator
{
  public:
    TMVTCCreator(int point_id) : TCreator(point_id, TCreatePoint())
    {
      *this << "MVTC";
    };
};

class TETLCreator : public TCreator
{
  public:
    TETLCreator(int point_id) : TCreator(point_id, TCreatePoint())
    {
      *this << "ETL";
    };
};

} //namespace TypeB

#endif

