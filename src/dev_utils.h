#ifndef _DEV_UTILS_H_
#define _DEV_UTILS_H_

#include "dev_consts.h"
#include "astra_consts.h"
#include "exceptions.h"
#include <string>
#include <list>
#include <vector>
#include <boost/optional/optional.hpp>
#include "basic.h"
#include "astra_utils.h"

ASTRA::TDevOperType DecodeDevOperType(std::string s);
ASTRA::TDevFmtType DecodeDevFmtType(std::string s);
std::string EncodeDevOperType(ASTRA::TDevOperType s);
std::string EncodeDevFmtType(ASTRA::TDevFmtType s);

ASTRA::TDevClassType getDevClass(const ASTRA::TOperMode desk_mode,
                                 const std::string &env_name);

std::string getDefaultDevModel(const ASTRA::TOperMode desk_mode,
                               const ASTRA::TDevClassType dev_class);

//bcbp_begin_idx - позиция первого байта штрих-кода
//airline_use_begin_idx - позиция первого байта <For individual airline use> первого сегмента
//airline_use_end_idx - позиция, следующая за последним байтом <For individual airline use> первого сегмента
void checkBCBP_M(const std::string &bcbp,
                 const std::string::size_type bcbp_begin_idx,
                 std::string::size_type &airline_use_begin_idx,
                 std::string::size_type &airline_use_end_idx);


namespace NamesBCBPData
{
    enum DocType{
        boarding_pass, itenirary_receipt
    };
    enum FreeBaggage
    {
        kg, pound, pc
    };

}



namespace BCBPPosAndSizes {

struct TConstPos
{
    unsigned int begin;
    unsigned int end;
    TConstPos(unsigned int _begin,  unsigned int length)
    { begin = _begin; end = begin + length;
    }
    TConstPos(TConstPos x,  unsigned int length)
    {
        begin = x.end; end = begin + length;
    }
    int size() const;
};
}

namespace  BCBPSectionsEnums {
    enum PassengerDescr{
        adult, male, female, child, infant, nobody, adult_with_infant, unaccompanied, future_industry_use
    };
    enum InternationalDocVerification
    {
        non_required, required, performed
    };

    enum PassengerClass
    {
        first, business, econom
    };
    enum DocType{
        boarding_pass, itenirary_receipt
    };
    enum FreeBaggage
    {
        kg, pound, pc
    };
    enum SourceOfIssuance
    {   web, kiosk, transfer_kiosk, remote_kiosk, mobile_device, airport_agent, town_agent, third_party_vendor
    };

    inline std::string to_string(PassengerDescr e)
    {
        switch(e)
        {   case adult: return "adult";
            case male: return "male";
            case female: return "female";
            case child: return "child";
            case infant:return "infant";
            case nobody: return "nobody";
            case adult_with_infant: return "adult with infant";
            case unaccompanied: return "unaccompanied";
            case future_industry_use: return "future industry use";
        }
        return "Incorrect data";
    }
    inline std::string to_string(InternationalDocVerification e)
    {
        switch(e)
        {   case non_required: return "non required";
            case required: return "required";
            case performed: return "performed";
        }
        return "Incorrect data";
    }
    inline std::string to_string(PassengerClass e)
    {
        switch(e)
        {   case first: return "first class";
            case business: return "business class";
            case econom: return "econom class";
        }
        return "Incorrect data";
    }

    inline std::string to_string(DocType e)
    {
        switch(e)
        {   case boarding_pass: return "boarding pass";
            case itenirary_receipt: return "itenirary receipt";
        }
        return "Incorrect data";
    }

    inline std::string to_string(FreeBaggage e)
    {
        switch(e)
        {   case kg: return "kilogramm";
            case pound: return "pound";
            case pc: return "pice concept";
        }
        return "Incorrect data";
    }
    inline std::string to_string(SourceOfIssuance e)
    {
        switch(e)
        {   case web: return "web";
            case kiosk: return "kiosk";
            case transfer_kiosk: return "transfer kiosk";
            case remote_kiosk: return "remote kiosk";
            case mobile_device:return "mobile device";
            case airport_agent: return "airport agent";
            case town_agent: return "town agent";
            case third_party_vendor: return "third party vendor";
        }
        return "Incorrect data";
    }
    inline std::string to_string(boost::optional<std::pair<int, BCBPSectionsEnums::FreeBaggage>> e)
    { if(e == boost::none) return "Void data";
      return std::to_string((*e).first) + " " + to_string((*e).second);
    }
    template <class T>
    std::string to_string(boost::optional<T> e)
    {   return e == boost::none ? "void value" : to_string(*e);
    }
    inline std::string to_string(boost::optional<int> e)
    {   return e == boost::none ? "void value" : std::to_string(*e);
    }
    inline std::string to_string(boost::optional<bool> e)
    {   return e == boost::none ? "void value" : std::to_string(*e);
    }

    inline std::string to_string(std::vector<std::string> x)
    {   if(!x.size()) return "";
        std::string  ret;
        for(unsigned int i = 0; i<x.size(); i++)
           ret+="\"" + x[i] + "\", ";
        return ret;
    }
    inline std::string to_string(boost::optional<BASIC::TDateTime> e)
    { return e == boost::none ? "void value" : BASIC::DateTimeToStr(*e, "dd.mm.yy", true);
    }

}


class BCBPInternalWork
{
public:
    std::vector<std::string> warnings;
    bool found_cyrilic, found_lower_case;
    std::string write_section_str(const std::string& field, const std::string& field_type, const std::string& descr, int section);
    static std::string invalid_format(const std::string& x);
    static std::string invalid_format(const std::string& field, const std::string& what);
protected:
    inline char get_xdigit(char x)
    {   if(x >= '0' && x <= '9') return x - '0';
        if(x >= 'A' && x <= 'F') return x - 'A' + 10;
        return -1;
    }
    static const std::string meet_null_term_symb;
    const std::string small_data_size(int i);


    template<class T, bool allow_nums = true, bool allow_non_found = true> boost::optional<T>  get_enum_opt(const std::string& x, int start, std::string& err, const std::string& test);

    template<bool allow_non_found = true>
    unsigned int get_int(const std::string& x, BCBPPosAndSizes::TConstPos pos, std::string& err, unsigned int allow_min = 0, unsigned int allow_max = 0xFFFFFFFF);

    template<class T, bool allow_nums = true, bool allow_non_found = true> boost::optional<T>  get_enum_opt(const std::string& x, BCBPPosAndSizes::TConstPos pos, std::string& err, const std::string& test);

    template<bool allow_nums, bool allow_non_found = true>
    std::string get_alfa_chars_str(const std::string& x, BCBPPosAndSizes::TConstPos pos, std::string& err, const std::string special_symbols_allowed = "");

    template<bool allow_non_num_alfa, bool allow_non_found = true>
    char get_char(const std::string& x, BCBPPosAndSizes::TConstPos pos, std::string& err);

    bool bad_symbol(char x,std::string& err);

    void process_err(const std::string& field,  const std::string& field_type, const std::string& descr, int section = -1);
    inline void test_on_warning(char x);


    template<bool allow_nums, bool allow_non_found = true>
    std::string get_alfa_chars_str(const std::string& x, int start, int end, std::string& err, const std::string special_symbols_allowed = "");

    template<bool allow_non_num_alfa, bool allow_non_found = true>
    char get_char(const std::string& x, int pos, std::string& err);

    template<bool allow_non_found = true>
    unsigned int get_int(const std::string& x, unsigned int start, unsigned int end,std::string& err, unsigned int allow_min = 0, unsigned int allow_max = 0xFFFFFFFF);

    template<bool allow_non_found = true>
    int get_hex(const std::string& x, unsigned int start,std::string& err);


    std::string delete_ending_blanks(const std::string& x);
    std::string delete_all_blanks(std::string& x);
    BCBPInternalWork()
    { found_cyrilic = found_lower_case = false;
    }


};

/*class BCBPData
{
    bool is_multi_segment_wait;
    char num_segments_wait;

public:
    bool electronic_ticket_indicator;
   std::string format_code, passenger_name, passenger_surname, security_data;
    char type_of_security_data;
    char passenger_description, source_of_checkin, source_of_boarding_pass_issuance, date_of_boarding_pass_issuance;
    NamesBCBPData::DocType doc_type;
   std::string airline_host_code;
   std::string baggage_tags[3];
    int version_number;
    std::vector<BCPBPSegment> segments;
    std::vector<short> airline_code;


};*/

class BCBPUniqueSections
{


  public:
    void mandatory_size_check(int i);
    std::string mandatory;
    std::string conditional;
    std::string security;

    void clear()
    {
      mandatory.clear();
      conditional.clear();
      security.clear();
    }

    int conditionalSize() const; //conditional
    int securitySize() const; //security

    int numberOfLigs() const;
    std::pair<std::string, std::string> passengerName() const;

};

std::ostream& operator<<(std::ostream& os, const BCBPUniqueSections&);

class BCBPRepeatedSections
{
  public:
    std::string mandatory;
    std::string conditional;
    std::string individual;

    void clear()
    {
      mandatory.clear();
      conditional.clear();
      individual.clear();
    }
    int variableSize(int seg) const; //conditional+individual
    int conditionalSize(int seg) const; //conditional

    std::string operatingCarrierPNRCode() const;
    std::string fromCityAirpCode() const;
    std::string toCityAirpCode() const;
    std::string operatingCarrierDesignator() const;
    std::pair<int, std::string> flightNumber() const;
    int dateOfFlight() const;
    std::pair<int, std::string> checkinSeqNumber() const;
};

std::ostream& operator<<(std::ostream& os, const BCBPRepeatedSections&);



class BCBPSections : public  BCBPInternalWork
{

    void clear()
    {
      unique.clear();
      repeated.clear();
    }
    void check_i(int i);
  public:

    struct Baggage_plate_nums
    {
        int64_t basic_number;
        unsigned short consequense;
        Baggage_plate_nums(int64_t _basic_number, unsigned short _consequense);
    };


    static int fieldSize(const std::string &s,
                         const EXCEPTIONS::EConvertError &e);
    static std::string substr_plus(const std::string &bcbp,
                                   std::string::size_type &idx,
                                   const std::string::size_type &len,
                                   const EXCEPTIONS::EConvertError &e);
    static std::string substr(const std::string &bcbp,
                              const std::string::size_type &idx,
                              const std::string::size_type &len,
                              const EXCEPTIONS::EConvertError &e);


    BCBPUniqueSections unique;
    std::vector<BCBPRepeatedSections> repeated;


    static void get(const std::string &bcbp,
                    const std::string::size_type bcbp_begin_idx,
                    const std::string::size_type bcbp_end_idx,
                    BCBPSections &sections,
                    bool only_mandatory=false);


   std::string airline_host_code;
   std::string baggage_tags[3];
    int version_number;
    bool electronic_ticket_indicator();

    char type_of_security_data();
    std::string security();

    boost::optional<BCBPSectionsEnums::PassengerDescr> passenger_description();
    boost::optional<BCBPSectionsEnums::SourceOfIssuance> source_of_checkin();
    boost::optional<BCBPSectionsEnums::DocType> doc_type();
    boost::optional<int> version();

    boost::optional<BCBPSectionsEnums::SourceOfIssuance> source_of_boarding_pass_issuance();

    boost::optional<BASIC::TDateTime> date_of_boarding_pass_issuance();
    std::string airline_of_boarding_pass_issuance();

    std::string operatingCarrierPNR(int i);

    std::string from_city_airport(int i);

    std::string to_city_airport(int i);

    std::string operating_carrier_designator(int i);

    std::string flight_number(int i);

    boost::optional<BCBPSectionsEnums::PassengerClass> compartment_code(int i);

    std::string seat_number(int i);

    std::string check_in_seq_number(int i);

    char passenger_status(int i);

    std::string doc_serial_num(int i);

    std::string marketing_carrier_designator(int i);

    std::string frequent_flyer_airline_designator(int i);

    std::string frequent_flyer_num(int i);

    boost::optional<bool> selectee(int i);

    char international_doc_verification(int i);
    std::vector<Baggage_plate_nums> baggage_plate_nums();
    std::vector<std::string> baggage_plate_nums_as_str();

    char id_ad(int i);



    int date_of_flight(int i);

    boost::optional<int> airline_num_code(int i);


    boost::optional<std::pair<int, BCBPSectionsEnums::FreeBaggage>> free_baggage_allowance(int i);
    boost::optional<bool> fast_track(int i);



    std::string airline_specific();

    int num_repeated_sections();

};

std::ostream& operator<<(std::ostream& os, const BCBPSections&);

int bcbp_test(int argc,char **argv);

class BCPBPSegment //Содержит поля, которые могут быть "repeated"
{
    public:
    std::string pnr_code, from_city_airport, to_city_airport, operating_carrier_designator, flight_number,
    compartment_code, seat_number, check_in_seq_number, passenger_status;
    std::string doc_serial_num,  marketing_carrier_designator, frequent_flyer_airline_designator, frequent_flyer_num;
    char selectee, international_doc_verification, id_ad;
    int date, airline;
    boost::optional<bool> fast_track;
   std::string airline_specific;
};



#endif
