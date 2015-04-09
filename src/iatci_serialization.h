#pragma once

#include "iatci_types.h"

#include <serverlib/dates.h>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_free.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/optional.hpp>

namespace boost
{
namespace serialization
{

namespace {

class OriginatorDetailsAccessor: private iatci::OriginatorDetails
{
public:
    // for save
    explicit OriginatorDetailsAccessor(const iatci::OriginatorDetails& o)
        : iatci::OriginatorDetails(o)
    {}

    // for load
    OriginatorDetailsAccessor()
    {}

    iatci::OriginatorDetails& get() { return *this; }

    using iatci::OriginatorDetails::m_airline;
    using iatci::OriginatorDetails::m_point;
};

}//namespace

/*****
 * OriginatorDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::OriginatorDetails& par, const unsigned int version)
{
    OriginatorDetailsAccessor acc(par);
    ar & acc.m_airline & acc.m_point;
}

template<class Archive>
inline void load(Archive& ar, iatci::OriginatorDetails& par, const unsigned int version)
{
    OriginatorDetailsAccessor acc;
    ar & acc.m_airline & acc.m_point;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::OriginatorDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class FlightDetailsAccessor: private iatci::FlightDetails
{
public:
    // for save
    explicit FlightDetailsAccessor(const iatci::FlightDetails& f)
        : iatci::FlightDetails(f)
    {}

    // for load
    FlightDetailsAccessor()
    {}

    iatci::FlightDetails& get() { return *this; }

    using iatci::FlightDetails::m_airline;
    using iatci::FlightDetails::m_flightNum;
    using iatci::FlightDetails::m_depPoint;
    using iatci::FlightDetails::m_arrPoint;
    using iatci::FlightDetails::m_depDate;
    using iatci::FlightDetails::m_arrDate;
    using iatci::FlightDetails::m_depTime;
    using iatci::FlightDetails::m_arrTime;
    using iatci::FlightDetails::m_boardingTime;
};

}//namespace

/*****
 * FlightDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::FlightDetails& par, const unsigned int version)
{
    FlightDetailsAccessor acc(par);
    ar & acc.m_airline;
    ar & acc.m_depPoint & acc.m_arrPoint;
    std::string depDate = Dates::ddmmyyyy(acc.m_depDate),
                arrDate = Dates::ddmmyyyy(acc.m_arrDate);
    std::string depTime = Dates::hh24mi(acc.m_depTime),
                arrTime = Dates::hh24mi(acc.m_arrTime),
                brdTime = Dates::hh24mi(acc.m_boardingTime);

    ar & depDate & depTime;
    ar & arrDate & arrTime;
    ar & brdTime;
    unsigned flNum = acc.m_flightNum.get();
    ar & flNum;
}

template<class Archive>
inline void load(Archive& ar, iatci::FlightDetails& par, const unsigned int version)
{
    FlightDetailsAccessor acc;
    ar & acc.m_airline;
    ar & acc.m_depPoint & acc.m_arrPoint;
    std::string depDate, arrDate;
    std::string depTime, arrTime;
    std::string brdTime;
    ar & depDate & depTime;
    ar & arrDate & arrTime;
    ar & brdTime;

    acc.m_depDate = Dates::ddmmyyyy(depDate);
    acc.m_arrDate = Dates::ddmmyyyy(arrDate);
    acc.m_depTime = Dates::hh24mi(depTime);
    acc.m_arrTime = Dates::hh24mi(arrTime);
    acc.m_boardingTime = Dates::hh24mi(brdTime);
    unsigned flNum = 0;
    ar & flNum;
    acc.m_flightNum = Ticketing::FlightNum_t(flNum);
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::FlightDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class PaxDetailsAccessor: private iatci::PaxDetails
{
public:
    // for save
    explicit PaxDetailsAccessor(const iatci::PaxDetails& p)
        : iatci::PaxDetails(p)
    {}

    // for load
    PaxDetailsAccessor()
    {}

    iatci::PaxDetails& get() { return *this; }

    using iatci::PaxDetails::m_surname;
    using iatci::PaxDetails::m_name;
    using iatci::PaxDetails::m_type;
    using iatci::PaxDetails::m_qryRef;
    using iatci::PaxDetails::m_respRef;
};

}//namespace

/*****
 * PaxDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::PaxDetails& par, const unsigned int version)
{
    PaxDetailsAccessor acc(par);
    ar & acc.m_surname & acc.m_name & acc.m_type & acc.m_respRef & acc.m_qryRef;
}

template<class Archive>
inline void load(Archive& ar, iatci::PaxDetails& par, const unsigned int version)
{
    PaxDetailsAccessor acc;
    ar & acc.m_surname & acc.m_name & acc.m_type & acc.m_respRef & acc.m_qryRef;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::PaxDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class ReservationDetailsAccessor: private iatci::ReservationDetails
{
public:
    // for save
    explicit ReservationDetailsAccessor(const iatci::ReservationDetails& r)
        : iatci::ReservationDetails(r)
    {}

    // for load
    ReservationDetailsAccessor()
    {}

    iatci::ReservationDetails& get() { return *this; }

    using iatci::ReservationDetails::m_rbd;
};

}//namespace

/*****
 * ReservationDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::ReservationDetails& par, const unsigned int version)
{
    ReservationDetailsAccessor acc(par);
    ar & acc.m_rbd;
}

template<class Archive>
inline void load(Archive& ar, iatci::ReservationDetails& par, const unsigned int version)
{
    ReservationDetailsAccessor acc;
    ar & acc.m_rbd;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::ReservationDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class SeatDetailsAccessor: private iatci::SeatDetails
{
public:
    // for save
    explicit SeatDetailsAccessor(const iatci::SeatDetails& s)
        : iatci::SeatDetails(s)
    {}

    // for load
    SeatDetailsAccessor()
    {}

    const iatci::SeatDetails& get() const { return *this; }

    using iatci::SeatDetails::m_smokeInd;
    using iatci::SeatDetails::m_characteristics;
};

}//namespace

/*****
 * SeatDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::SeatDetails& par, const unsigned int version)
{
    SeatDetailsAccessor acc(par);
    ar & acc.m_smokeInd & acc.m_characteristics;
}

template<class Archive>
inline void load(Archive& ar, iatci::SeatDetails& par, const unsigned int version)
{
    SeatDetailsAccessor acc;
    ar & acc.m_smokeInd & acc.m_characteristics;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::SeatDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class FlightSeatDetailsAccessor: private iatci::FlightSeatDetails
{
public:
    // for save
    explicit FlightSeatDetailsAccessor(const iatci::FlightSeatDetails& f)
        : iatci::FlightSeatDetails(f)
    {}

    // for load
    FlightSeatDetailsAccessor()
    {}

    iatci::FlightSeatDetails& get() { return *this; }

    using iatci::FlightSeatDetails::m_seat;
    using iatci::FlightSeatDetails::m_cabinClass;
    using iatci::FlightSeatDetails::m_securityId;
};

}//namespace

/*****
 * FlightSeatDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::FlightSeatDetails& par, const unsigned int version)
{
    FlightSeatDetailsAccessor acc(par);
    ar & boost::serialization::base_object<iatci::SeatDetails>(acc.get());
    ar & acc.m_seat & acc.m_cabinClass & acc.m_securityId;
}

template<class Archive>
inline void load(Archive& ar, iatci::FlightSeatDetails& par, const unsigned int version)
{
    FlightSeatDetailsAccessor acc;
    ar & boost::serialization::base_object<iatci::SeatDetails>(acc.get());
    ar & acc.m_seat & acc.m_cabinClass & acc.m_securityId;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::FlightSeatDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class BaggageDetailsAccessor: private iatci::BaggageDetails
{
public:
    // for save
    explicit BaggageDetailsAccessor(const iatci::BaggageDetails& b)
        : iatci::BaggageDetails(b)
    {}

    // for load
    BaggageDetailsAccessor()
    {}

    iatci::BaggageDetails& get() { return *this; }

    using iatci::BaggageDetails::m_numOfPieces;
    using iatci::BaggageDetails::m_weight;
};

}//namespace

/*****
 * BaggageDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::BaggageDetails& par, const unsigned int version)
{
    BaggageDetailsAccessor acc(par);
    ar & acc.m_numOfPieces & acc.m_weight;
}

template<class Archive>
inline void load(Archive& ar, iatci::BaggageDetails& par, const unsigned int version)
{
    BaggageDetailsAccessor acc;
    ar & acc.m_numOfPieces & acc.m_weight;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::BaggageDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class CascadeHostDetailsAccessor: private iatci::CascadeHostDetails
{
public:
    // for save
    explicit CascadeHostDetailsAccessor(const iatci::CascadeHostDetails& d)
        : iatci::CascadeHostDetails(d)
    {}

    // for load
    CascadeHostDetailsAccessor()
    {}

    iatci::CascadeHostDetails& get() { return *this; }

    using iatci::CascadeHostDetails::m_originAirline;
    using iatci::CascadeHostDetails::m_originPoint;
    using iatci::CascadeHostDetails::m_hostAirlines;
};

}//namespace

/*****
 * CascadeHostDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::CascadeHostDetails& par, const unsigned int version)
{
    CascadeHostDetailsAccessor acc(par);
    ar & acc.m_originAirline & acc.m_originPoint & acc.m_hostAirlines;
}

template<class Archive>
inline void load(Archive& ar, iatci::CascadeHostDetails& par, const unsigned int version)
{
    CascadeHostDetailsAccessor acc;
    ar & acc.m_originAirline & acc.m_originPoint & acc.m_hostAirlines;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::CascadeHostDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class ErrorDetailsAccessor: private iatci::ErrorDetails
{
public:
    // for save
    explicit ErrorDetailsAccessor(const iatci::ErrorDetails& e)
        : iatci::ErrorDetails(e)
    {}

    // for load
    ErrorDetailsAccessor()
    {}

    iatci::ErrorDetails& get() { return *this; }

    using iatci::ErrorDetails::m_errText;
};

}//namespace

/*****
 * ErrorDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::ErrorDetails& par, const unsigned int version)
{
    ErrorDetailsAccessor acc(par);
    ar & acc.m_errText;
}

template<class Archive>
inline void load(Archive& ar, iatci::ErrorDetails& par, const unsigned int version)
{
    ErrorDetailsAccessor acc;
    ar & acc.m_errText;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::ErrorDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class CkiResultAccessor: private iatci::Result
{
public:
    // for save
    explicit CkiResultAccessor(const iatci::Result& r)
        : iatci::Result(r)
    {}

    // for load
    CkiResultAccessor()
    {}

    Result& get() { return *this; }

    using iatci::Result::m_action;
    using iatci::Result::m_status;
    using iatci::Result::m_flight;
    using iatci::Result::m_pax;
    using iatci::Result::m_seat;
    using iatci::Result::m_cascadeDetails;
    using iatci::Result::m_errorDetails;
};

}//namespace

/*****
 * Result
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::Result& par, const unsigned int version)
{
    CkiResultAccessor acc(par);
    ar & acc.m_action & acc.m_status & acc.m_flight & acc.m_pax;
    ar & acc.m_seat & acc.m_cascadeDetails & acc.m_errorDetails;
}

template<class Archive>
inline void load(Archive& ar, iatci::Result& par, const unsigned int version)
{
    CkiResultAccessor acc;
    ar & acc.m_action & acc.m_status & acc.m_flight & acc.m_pax;
    ar & acc.m_seat & acc.m_cascadeDetails & acc.m_errorDetails;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::Result& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

}//namespace serialization
}//namespace boost
