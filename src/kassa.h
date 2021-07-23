#pragma once

#include <set>
#include <string>
#include "astra_types.h"
#include "astra_misc.h"

namespace Kassa {

namespace BagNorm {
int add(int id, const std::string& airline, std::optional<bool> pr_trfer, const std::string& city_dep,
        const std::string& city_arv, const std::string& pax_cat, const std::string& subclass,
        const std::string& cls, int flt_no, const std::string& craft,
        const std::string& trip_type, TDateTime first_date, TDateTime last_date,
        int bag_type, int amount, int weight, int per_unit, const std::string& norm_type,
        const std::string& extra, int tid);

void modifyById(int id, TDateTime last_date, int tid = ASTRA::NoExists);

void deleteById(int id, int tid = ASTRA::NoExists);

void copyBasic(const std::string& airline);
} // namespace BagNorm

namespace BagRate {
int add(int id, const std::string& airline, std::optional<bool> pr_trfer, const std::string& city_dep,
        const std::string& city_arv, const std::string& pax_cat, const std::string& subclass,
        const std::string& cls, int flt_no, const std::string& craft,
        const std::string& trip_type, TDateTime first_date, TDateTime last_date,
        int bag_type, int rate, const std::string& rate_cur, int min_weight,
        const std::string& extra, int tid);

void modifyById(int id, TDateTime last_date, int tid = ASTRA::NoExists);
void deleteById(int id, int tid = ASTRA::NoExists);
void copyBasic(const std::string& airline);
} // namespace BagRate

namespace ValueBagTax {
int add(int id, const std::string& airline, std::optional<bool> pr_trfer, const std::string& city_dep,
        const std::string& city_arv, TDateTime first_date, TDateTime last_date,
        int tax, int min_value, const std::string& min_value_cur, const std::string& extra, int tid);

void modifyById(int id, TDateTime last_date, int tid = ASTRA::NoExists);
void deleteById(int id, int tid = ASTRA::NoExists);
void copyBasic(const std::string& airline);
} // namespace ValueBagTax

namespace ExchangeRate {
int add(int id, const std::string& airline, int rate1, const std::string& cur1,
        int rate2, const std::string& cur2, TDateTime first_date, TDateTime last_date,
        const std::string& extra, int tid);

void modifyById(int id, TDateTime last_date, int tid = ASTRA::NoExists);
void deleteById(int id, int tid = ASTRA::NoExists);
void copyBasic(const std::string& airline);
} // namespace ExchangeRate

}
