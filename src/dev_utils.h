#ifndef _DEV_UTILS_H_
#define _DEV_UTILS_H_

#include "dev_consts.h"
#include "astra_consts.h"
#include "exceptions.h"
#include <string>
#include <list>

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

class BCBPUniqueSections
{
  public:
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

class BCBPSections
{
  public:
    BCBPUniqueSections unique;
    std::list<BCBPRepeatedSections> repeated;

    void clear()
    {
      unique.clear();
      repeated.clear();
    }

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
    static void get(const std::string &bcbp,
                    const std::string::size_type bcbp_begin_idx,
                    BCBPSections &sections,
                    bool only_mandatory=false);
};

std::ostream& operator<<(std::ostream& os, const BCBPSections&);


#endif
