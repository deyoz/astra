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
    using iatci::FlightDetails::m_depPort;
    using iatci::FlightDetails::m_arrPort;
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
    ar & acc.m_depPort & acc.m_arrPort;
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
    ar & acc.m_depPort & acc.m_arrPort;
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

class DocDetailsAccessor: private iatci::DocDetails
{
public:
    // for save
    explicit DocDetailsAccessor(const iatci::DocDetails& d)
        : iatci::DocDetails(d)
    {}

    // for load
    DocDetailsAccessor()
    {}

    iatci::DocDetails& get() { return *this; }

    using iatci::DocDetails::m_birthDate;
    using iatci::DocDetails::m_docType;
    using iatci::DocDetails::m_expiryDate;
    using iatci::DocDetails::m_gender;
    using iatci::DocDetails::m_issueCountry;
    using iatci::DocDetails::m_name;
    using iatci::DocDetails::m_secondName;
    using iatci::DocDetails::m_nationality;
    using iatci::DocDetails::m_no;
    using iatci::DocDetails::m_surname;
};

//

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
    using iatci::PaxDetails::m_withInftIndic;
};

}//namespace


/*****
 * DocDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::DocDetails& par, const unsigned int version)
{
    DocDetailsAccessor acc(par);
    std::string bd = Dates::ddmmyyyy(acc.m_birthDate),
                ed = Dates::ddmmyyyy(acc.m_expiryDate);

    ar & bd & ed & acc.m_docType;
    ar & acc.m_gender & acc.m_issueCountry & acc.m_name;
    ar & acc.m_nationality & acc.m_no & acc.m_surname;
    ar & acc.m_secondName;
}

template<class Archive>
inline void load(Archive& ar, iatci::DocDetails& par, const unsigned int version)
{
    DocDetailsAccessor acc;
    std::string bd, ed;

    ar & bd & ed & acc.m_docType;
    ar & acc.m_gender & acc.m_issueCountry & acc.m_name;
    ar & acc.m_nationality & acc.m_no & acc.m_surname;
    ar & acc.m_secondName;
    acc.m_birthDate  = Dates::ddmmyyyy(bd);
    acc.m_expiryDate = Dates::ddmmyyyy(ed);
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::DocDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

/*****
 * PaxDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::PaxDetails& par, const unsigned int version)
{
    PaxDetailsAccessor acc(par);
    ar & acc.m_surname & acc.m_name & acc.m_type
       & acc.m_respRef & acc.m_qryRef & acc.m_withInftIndic;
}

template<class Archive>
inline void load(Archive& ar, iatci::PaxDetails& par, const unsigned int version)
{
    PaxDetailsAccessor acc;
    ar & acc.m_surname & acc.m_name & acc.m_type
       & acc.m_respRef & acc.m_qryRef & acc.m_withInftIndic;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::PaxDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//namespace iatci {
//    BOOST_CLASS_VERSION(PaxDetails, 1)
//}//namespace iatci

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

    const iatci::ReservationDetails& get() const { return *this; }

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

class BaggageDetailsAccessor: private iatci::BaggageDetails
{
public:
    // for save
    explicit BaggageDetailsAccessor(const iatci::BaggageDetails& r)
        : iatci::BaggageDetails(r)
    {}

    // for load
    BaggageDetailsAccessor()
    {}

    const iatci::BaggageDetails& get() const { return *this; }

    using iatci::BaggageDetails::m_numOfPieces;
    using iatci::BaggageDetails::m_weight;
};

}//namespace

/*****
 * BaggageDetailsAccessor
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

class AddressDetailsAccessor: private iatci::AddressDetails
{
public:
    // for save
    explicit AddressDetailsAccessor(const iatci::AddressDetails& r)
        : iatci::AddressDetails(r)
    {}

    // for load
    AddressDetailsAccessor()
    {}

    const iatci::AddressDetails& get() const { return *this; }

    // TODO
};

}//namespace

/*****
 * AddressDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::AddressDetails& par, const unsigned int version)
{
    AddressDetailsAccessor acc(par);
}

template<class Archive>
inline void load(Archive& ar, iatci::AddressDetails& par, const unsigned int version)
{
    AddressDetailsAccessor acc;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::AddressDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class PaxGroupAccessor: private iatci::dcrcka::PaxGroup
{
public:
    // for save
    explicit PaxGroupAccessor(const iatci::dcrcka::PaxGroup& p)
        : iatci::dcrcka::PaxGroup(p)
    {}

    // for load
    PaxGroupAccessor()
    {}

    const iatci::dcrcka::PaxGroup& get() const { return *this; }

    using iatci::dcrcka::PaxGroup::m_pax;
    using iatci::dcrcka::PaxGroup::m_reserv;
    using iatci::dcrcka::PaxGroup::m_seat;
    using iatci::dcrcka::PaxGroup::m_baggage;
    using iatci::dcrcka::PaxGroup::m_service;
    using iatci::dcrcka::PaxGroup::m_doc;
    using iatci::dcrcka::PaxGroup::m_address;
    using iatci::dcrcka::PaxGroup::m_infant;
    using iatci::dcrcka::PaxGroup::m_infantDoc;
    using iatci::dcrcka::PaxGroup::m_infantSeat;
};

}//namespace

/*****
 * PaxGroup
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::dcrcka::PaxGroup& par, const unsigned int version)
{
    PaxGroupAccessor acc(par);
    ar & acc.m_pax & acc.m_reserv & acc.m_seat & acc.m_baggage;
    ar & acc.m_service & acc.m_doc & acc.m_address & acc.m_infant;
    ar & acc.m_infantDoc & acc.m_infantSeat;
}

template<class Archive>
inline void load(Archive& ar, iatci::dcrcka::PaxGroup& par, const unsigned int version)
{
    PaxGroupAccessor acc;
    ar & acc.m_pax & acc.m_reserv & acc.m_seat & acc.m_baggage;
    ar & acc.m_service & acc.m_doc & acc.m_address & acc.m_infant;
    ar & acc.m_infantDoc & acc.m_infantSeat;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::dcrcka::PaxGroup& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//namespace iatci {
//namespace dcrcka {
//    BOOST_CLASS_VERSION(PaxGroup, 1)
//}//namespace dcrcka
//}//namespace iatci

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

    using iatci::SeatDetails::m_seat;
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
    ar & acc.m_seat & acc.m_smokeInd & acc.m_characteristics;
}

template<class Archive>
inline void load(Archive& ar, iatci::SeatDetails& par, const unsigned int version)
{
    SeatDetailsAccessor acc;
    ar & acc.m_seat & acc.m_smokeInd & acc.m_characteristics;
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
    using iatci::FlightSeatDetails::m_regNo;
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
    ar & acc.m_seat & acc.m_cabinClass & acc.m_regNo;
}

template<class Archive>
inline void load(Archive& ar, iatci::FlightSeatDetails& par, const unsigned int version)
{
    FlightSeatDetailsAccessor acc;
    ar & boost::serialization::base_object<iatci::SeatDetails>(acc.get());
    ar & acc.m_seat & acc.m_cabinClass & acc.m_regNo;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::FlightSeatDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class RowRangeAccessor: private iatci::RowRange
{
public:
    // for save
    explicit RowRangeAccessor(const iatci::RowRange& r)
        : iatci::RowRange(r)
    {}

    // for load
    RowRangeAccessor()
    {}

    iatci::RowRange& get() { return *this; }

    using iatci::RowRange::m_firstRow;
    using iatci::RowRange::m_lastRow;
};

}//namespace

/*****
 * RowRange
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::RowRange& par, const unsigned int version)
{
    RowRangeAccessor acc(par);
    ar & acc.m_firstRow & acc.m_lastRow;
}

template<class Archive>
inline void load(Archive& ar, iatci::RowRange& par, const unsigned int version)
{
    RowRangeAccessor acc;
    ar & acc.m_firstRow & acc.m_lastRow;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::RowRange& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class SeatColumnDetailsAccessor: private iatci::SeatColumnDetails
{
public:
    // for save
    explicit SeatColumnDetailsAccessor(const iatci::SeatColumnDetails& s)
        : iatci::SeatColumnDetails(s)
    {}

    // for load
    SeatColumnDetailsAccessor()
    {}

    iatci::SeatColumnDetails& get() { return *this; }

    using iatci::SeatColumnDetails::m_column;
    using iatci::SeatColumnDetails::m_desc1;
    using iatci::SeatColumnDetails::m_desc2;
};

}//namespace

/*****
 * SeatColumnDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::SeatColumnDetails& par, const unsigned int version)
{
    SeatColumnDetailsAccessor acc(par);
    ar & acc.m_column & acc.m_desc1 & acc.m_desc2;
}

template<class Archive>
inline void load(Archive& ar, iatci::SeatColumnDetails& par, const unsigned int version)
{
    SeatColumnDetailsAccessor acc;
    ar & acc.m_column & acc.m_desc1 & acc.m_desc2;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::SeatColumnDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class RowDetailsAccessor: private iatci::RowDetails
{
public:
    // for save
    explicit RowDetailsAccessor(const iatci::RowDetails& r)
        : iatci::RowDetails(r)
    {}

    // for load
    RowDetailsAccessor()
    {}

    iatci::RowDetails& get() { return *this; }

    using iatci::RowDetails::m_row;
    using iatci::RowDetails::m_lOccupationDetails;
    using iatci::RowDetails::m_characteristic;
};

}//namespace

/*****
 * RowDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::RowDetails& par, const unsigned int version)
{
    RowDetailsAccessor acc(par);
    ar & acc.m_row & acc.m_lOccupationDetails & acc.m_characteristic;
}

template<class Archive>
inline void load(Archive& ar, iatci::RowDetails& par, const unsigned int version)
{
    RowDetailsAccessor acc;
    ar & acc.m_row & acc.m_lOccupationDetails & acc.m_characteristic;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::RowDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class CabinDetailsAccessor: private iatci::CabinDetails
{
public:
    // for save
    explicit CabinDetailsAccessor(const iatci::CabinDetails& c)
        : iatci::CabinDetails(c)
    {}

    // for load
    CabinDetailsAccessor()
    {}

    iatci::CabinDetails& get() { return *this; }

    using iatci::CabinDetails::m_classDesignator;
    using iatci::CabinDetails::m_rowRange;
    using iatci::CabinDetails::m_deck;
    using iatci::CabinDetails::m_defSeatOccupation;
    using iatci::CabinDetails::m_overwingArea;
    using iatci::CabinDetails::m_seatColumns;
    using iatci::CabinDetails::m_smokingArea;
};

}//namespace

/*****
 * CabinDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::CabinDetails& par, const unsigned int version)
{
    CabinDetailsAccessor acc(par);
    ar & acc.m_classDesignator & acc.m_rowRange & acc.m_deck;
    ar & acc.m_defSeatOccupation & acc.m_overwingArea & acc.m_seatColumns;
    ar & acc.m_smokingArea;
}

template<class Archive>
inline void load(Archive& ar, iatci::CabinDetails& par, const unsigned int version)
{
    CabinDetailsAccessor acc;
    ar & acc.m_classDesignator & acc.m_rowRange & acc.m_deck;
    ar & acc.m_defSeatOccupation & acc.m_overwingArea & acc.m_seatColumns;
    ar & acc.m_smokingArea;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::CabinDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class SeatOccupationDetailsAccessor: private iatci::SeatOccupationDetails
{
public:
    // for save
    explicit SeatOccupationDetailsAccessor(const iatci::SeatOccupationDetails& o)
        : iatci::SeatOccupationDetails(o)
    {}

    // for load
    SeatOccupationDetailsAccessor()
    {}

    iatci::SeatOccupationDetails& get() { return *this; }

    using iatci::SeatOccupationDetails::m_column;
    using iatci::SeatOccupationDetails::m_occupation;
    using iatci::SeatOccupationDetails::m_lCharacteristics;
};

}//namespace

/*****
 * SeatOccupationDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::SeatOccupationDetails& par, const unsigned int version)
{
    SeatOccupationDetailsAccessor acc(par);
    ar & acc.m_column & acc.m_occupation & acc.m_lCharacteristics;
}

template<class Archive>
inline void load(Archive& ar, iatci::SeatOccupationDetails& par, const unsigned int version)
{
    SeatOccupationDetailsAccessor acc;
    ar & acc.m_column & acc.m_occupation & acc.m_lCharacteristics;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::SeatOccupationDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {


class SeatRequestDetailsAccessor: private iatci::SeatRequestDetails
{
public:
    // for save
    explicit SeatRequestDetailsAccessor(const iatci::SeatRequestDetails& s)
        : iatci::SeatRequestDetails(s)
    {}

    // for load
    SeatRequestDetailsAccessor()
    {}

    iatci::SeatRequestDetails& get() { return *this; }

    using iatci::SeatRequestDetails::m_cabinClass;
};

}//namespace

/*****
 * SeatRequestDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::SeatRequestDetails& par, const unsigned int version)
{
    SeatRequestDetailsAccessor acc(par);
    ar & boost::serialization::base_object<iatci::SeatDetails>(acc.get());
    ar & acc.m_cabinClass;
}

template<class Archive>
inline void load(Archive& ar, iatci::SeatRequestDetails& par, const unsigned int version)
{
    SeatRequestDetailsAccessor acc;
    ar & boost::serialization::base_object<iatci::SeatDetails>(acc.get());
    ar & acc.m_cabinClass;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::SeatRequestDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class SeatmapDetailsAccessor: private iatci::SeatmapDetails
{
public:
    // for save
    explicit SeatmapDetailsAccessor(const iatci::SeatmapDetails& s)
        : iatci::SeatmapDetails(s)
    {}

    // for load
    SeatmapDetailsAccessor()
    {}

    iatci::SeatmapDetails& get() { return *this; }

    using iatci::SeatmapDetails::m_lCabin;
    using iatci::SeatmapDetails::m_lRow;
    using iatci::SeatmapDetails::m_seatRequest;
};

}//namespace

/*****
 * SeatmapDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::SeatmapDetails& par, const unsigned int version)
{
    SeatmapDetailsAccessor acc(par);
    ar & acc.m_lCabin & acc.m_lRow & acc.m_seatRequest;
}

template<class Archive>
inline void load(Archive& ar, iatci::SeatmapDetails& par, const unsigned int version)
{
    SeatmapDetailsAccessor acc;
    ar & acc.m_lCabin & acc.m_lRow & acc.m_seatRequest;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::SeatmapDetails& par, const unsigned int version)
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
    using iatci::CascadeHostDetails::m_originPort;
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
    ar & acc.m_originAirline & acc.m_originPort & acc.m_hostAirlines;
}

template<class Archive>
inline void load(Archive& ar, iatci::CascadeHostDetails& par, const unsigned int version)
{
    CascadeHostDetailsAccessor acc;
    ar & acc.m_originAirline & acc.m_originPort & acc.m_hostAirlines;
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

    using iatci::ErrorDetails::m_errCode;
    using iatci::ErrorDetails::m_errDesc;
};

}//namespace

/*****
 * ErrorDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::ErrorDetails& par, const unsigned int version)
{
    ErrorDetailsAccessor acc(par);
    ar & acc.m_errCode & acc.m_errDesc;
}

template<class Archive>
inline void load(Archive& ar, iatci::ErrorDetails& par, const unsigned int version)
{
    ErrorDetailsAccessor acc;
    ar & acc.m_errCode & acc.m_errDesc;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::ErrorDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class WarningDetailsAccessor: private iatci::WarningDetails
{
public:
    // for save
    explicit WarningDetailsAccessor(const iatci::WarningDetails& e)
        : iatci::WarningDetails(e)
    {}

    // for load
    WarningDetailsAccessor()
    {}

    iatci::WarningDetails& get() { return *this; }

    using iatci::WarningDetails::m_warningCode;
    using iatci::WarningDetails::m_warningDesc;
};

}//namespace

/*****
 * WarningDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::WarningDetails& par, const unsigned int version)
{
    WarningDetailsAccessor acc(par);
    ar & acc.m_warningCode & acc.m_warningDesc;
}

template<class Archive>
inline void load(Archive& ar, iatci::WarningDetails& par, const unsigned int version)
{
    WarningDetailsAccessor acc;
    ar & acc.m_warningCode & acc.m_warningDesc;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::WarningDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class EquipmentDetailsAccessor: private iatci::EquipmentDetails
{
public:
    // for save
    explicit EquipmentDetailsAccessor(const iatci::EquipmentDetails& e)
        : iatci::EquipmentDetails(e)
    {}

    // for load
    EquipmentDetailsAccessor()
    {}

    iatci::EquipmentDetails& get() { return *this; }

    using iatci::EquipmentDetails::m_equipment;
};

}//namespace

/*****
 * EquipmentDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::EquipmentDetails& par, const unsigned int version)
{
    EquipmentDetailsAccessor acc(par);
    ar & acc.m_equipment;
}

template<class Archive>
inline void load(Archive& ar, iatci::EquipmentDetails& par, const unsigned int version)
{
    EquipmentDetailsAccessor acc;
    ar & acc.m_equipment;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::EquipmentDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class SsrInfoAccessor: private iatci::ServiceDetails::SsrInfo
{
public:
    // for save
    explicit SsrInfoAccessor(const iatci::ServiceDetails::SsrInfo& par)
        : iatci::ServiceDetails::SsrInfo(par)
    {}

    SsrInfoAccessor()
    {}

    iatci::ServiceDetails::SsrInfo& get() { return *this; }

    using iatci::ServiceDetails::SsrInfo::m_ssrCode;
    using iatci::ServiceDetails::SsrInfo::m_airline;
    using iatci::ServiceDetails::SsrInfo::m_ssrText;
    using iatci::ServiceDetails::SsrInfo::m_isInfantTicket;
    using iatci::ServiceDetails::SsrInfo::m_quantity;
    using iatci::ServiceDetails::SsrInfo::m_freeText;
};

}//namespace

/*****
 * SsrInfo
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::ServiceDetails::SsrInfo& par, const unsigned int version)
{
    SsrInfoAccessor acc(par);
    ar & acc.m_ssrCode & acc.m_airline & acc.m_ssrText;
    ar & acc.m_quantity & acc.m_isInfantTicket & acc.m_freeText;
}

template<class Archive>
inline void load(Archive& ar, iatci::ServiceDetails::SsrInfo& par, const unsigned int version)
{
    SsrInfoAccessor acc;
    ar & acc.m_ssrCode & acc.m_airline & acc.m_ssrText;
    ar & acc.m_quantity & acc.m_isInfantTicket & acc.m_freeText;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::ServiceDetails::SsrInfo& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}


//---------------------------------------------------------------------------------------

namespace {

class ServiceDetailsAccessor: private iatci::ServiceDetails
{
public:
    // for save
    explicit ServiceDetailsAccessor(const iatci::ServiceDetails& par)
        : iatci::ServiceDetails(par)
    {}

    // for load
    ServiceDetailsAccessor()
    {}

    iatci::ServiceDetails& get() { return *this; }

    using iatci::ServiceDetails::m_osi;
    using iatci::ServiceDetails::m_lSsr;
};

}//namespace

/*****
 * ServiceDetails
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::ServiceDetails& par, const unsigned int version)
{
    ServiceDetailsAccessor acc(par);
    ar & acc.m_osi & acc.m_lSsr;
}

template<class Archive>
inline void load(Archive& ar, iatci::ServiceDetails& par, const unsigned int version)
{
    ServiceDetailsAccessor acc;
    ar & acc.m_osi & acc.m_lSsr;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::ServiceDetails& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//---------------------------------------------------------------------------------------

namespace {

class CkiResultAccessor: private iatci::dcrcka::Result
{
public:
    // for save
    explicit CkiResultAccessor(const iatci::dcrcka::Result& r)
        : iatci::dcrcka::Result(r)
    {}

    // for load
    CkiResultAccessor()
    {}

    Result& get() { return *this; }

    using iatci::dcrcka::Result::m_action;
    using iatci::dcrcka::Result::m_status;
    using iatci::dcrcka::Result::m_flight;
    using iatci::dcrcka::Result::m_paxGroups;
    using iatci::dcrcka::Result::m_seatmap;
    using iatci::dcrcka::Result::m_cascade;
    using iatci::dcrcka::Result::m_error;
    using iatci::dcrcka::Result::m_warning;
    using iatci::dcrcka::Result::m_equipment;
};

}//namespace

/*****
 * Result
 *****/
template<class Archive>
inline void save(Archive& ar, const iatci::dcrcka::Result& par, const unsigned int version)
{
    CkiResultAccessor acc(par);
    ar & acc.m_action & acc.m_status & acc.m_flight & acc.m_paxGroups;
    ar & acc.m_seatmap & acc.m_cascade & acc.m_error;
    ar & acc.m_warning & acc.m_equipment;
}

template<class Archive>
inline void load(Archive& ar, iatci::dcrcka::Result& par, const unsigned int version)
{
    CkiResultAccessor acc;
    ar & acc.m_action & acc.m_status & acc.m_flight & acc.m_paxGroups;
    ar & acc.m_seatmap & acc.m_cascade & acc.m_error;
    ar & acc.m_warning & acc.m_equipment;
    par = acc.get();
}

template<class Archive>
inline void serialize(Archive& ar, iatci::dcrcka::Result& par, const unsigned int version)
{
    boost::serialization::split_free(ar, par, version);
}

//namespace iatci {
//namespace dcrcka {
//    BOOST_CLASS_VERSION(Result, 1)
//}//namespace dcrcka
//}//namespace iatci

}//namespace serialization
}//namespace boost
