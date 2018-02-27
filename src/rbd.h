#pragma once

#include <string>

#include "oralib.h"
#include "astra_misc.h"

class TRbdKey
{
  public:
    std::string fare_class;

    TRbdKey() {}
    TRbdKey(const std::string& _fare_class) : fare_class(_fare_class) {}

    void clear()
    {
      fare_class.clear();
    }

    bool operator == (const TRbdKey &key) const
    {
      return fare_class==key.fare_class;
    }

    bool operator < (const TRbdKey &key) const
    {
      return fare_class<key.fare_class;
    }
};

class TCompartment
{
  private:
    std::string _value;

  public:
    TCompartment(const std::string& compartment) : _value(compartment) {}

    const std::string& value() const { return _value; }

    bool operator == (const TCompartment &compartment) const
    {
      return _value==compartment._value;
    }

    static boost::optional<TCompartment> fromDB(TQuery& Qry)
    {
      if (Qry.FieldIsNULL("compartment")) return boost::none;
      return TCompartment(Qry.FieldAsString("compartment"));
    }
};

class TSubclassGroup
{
  private:
    int _value;

  public:
    TSubclassGroup(const int& subclassGroup) : _value(subclassGroup) {}

    const int& value() const { return _value; }

    bool operator == (const TSubclassGroup &subclassGroup) const
    {
      return _value==subclassGroup._value;
    }

    static boost::optional<TSubclassGroup> fromDB(TQuery& Qry)
    {
      if (Qry.FieldIsNULL("subclass_group")) return boost::none;
      return TSubclassGroup(Qry.FieldAsInteger("subclass_group"));
    }
};

template<class T>
class TRbdItem : public TRbdKey
{
  public:
    boost::optional<T> compartment;

    TRbdItem() { clear(); }

    void clear()
    {
      TRbdKey::clear();
      compartment=boost::none;
    }

    void fromDB(TQuery& Qry)
    {
      clear();
      fare_class=Qry.FieldAsString("fare_class");
      compartment=T::fromDB(Qry);
    }
};

class TFlightRbd
{
  private:
    std::map<TRbdKey, TRbdItem<TSubclassGroup> > base_classes, fare_classes;

  public:
    TFlightRbd(const TTripInfo& flt);

    boost::optional<int> getMaxRbdPriority(const TTripInfo& flt) const;

    void clear()
    {
      base_classes.clear();
      fare_classes.clear();
    }

    bool empty() const
    {
      return base_classes.empty() && fare_classes.empty();
    }

    boost::optional<TSubclassGroup> getSubclassGroup(const std::string& subcls, const std::string& cls) const;

    void dump(const std::string& whence) const;
};
