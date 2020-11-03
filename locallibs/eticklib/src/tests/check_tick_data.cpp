#ifdef XP_TESTING

#include "etick/tick_data.h"
#include "etick/exceptions.h"
#include "etick/etick_msg.h"

void init_tick_data_cpp()
{}

#include <serverlib/checkunit.h>
#include <serverlib/xp_test_utils.h>

namespace
{
using namespace Ticketing;

START_TEST(check_hist_code_1)
{
    HistCode hc;

    fail_if(hc, "HistCode inited");

    if(!hc)
        ;// ok
    else
        fail_if(hc, "HistCode inited");

    fail_unless(!hc.subCode());
}END_TEST

START_TEST(check_hist_code_2)
{
    HistCode hc("ISSUE");

    fail_if(!hc, "HistCode not inited");

    fail_unless(hc->code() == std::string("ISSUE"));
    fail_unless(hc->codeInt() == HistCode::Issue);
    fail_unless(hc->description() == std::string("Issue"));
    fail_unless(hc->ediCode() == "130");
    fail_unless(!hc.subCode());
}END_TEST

START_TEST(check_hist_code_3)
{
    HistCode hc(HistCode::Exchange);

    fail_if(!hc, "HistCode not inited");

    fail_unless(hc->code() == std::string("EXCHANGE"));
    fail_unless(hc->codeInt() == HistCode::Exchange);
    fail_unless(hc->description() == std::string("Exchange"));
    fail_unless(hc->ediCode() == "134");
    fail_unless(!hc.subCode());
}END_TEST

START_TEST(check_hist_code_4)
{
    HistCode hc(HistCode::Refund, HistCode::Exchange);

    fail_if(!hc, "HistCode not inited");

    fail_unless(hc->code() == std::string("REFUND"));
    fail_unless(hc->codeInt() == HistCode::Refund);
    fail_unless(hc->description() == std::string("Refund"));
    fail_unless(hc->ediCode() == "135");

    fail_unless(!!hc.subCode());
    fail_unless(hc.subCode()->code() == std::string("EXCHANGE"));
    fail_unless(hc.subCode()->codeInt() == HistCode::Exchange);
}END_TEST

START_TEST(check_hist_code_5)
{
    HistSubCode hc;

    fail_if(hc, "HistSubCode inited");
}END_TEST

START_TEST(check_hist_code_6)
{
    HistSubCode hc(HistCode::SystemCancel);

    fail_if(!hc, "HistSubCode not inited");

    fail_unless(hc->code() == std::string("SYS_CANCEL"));
    fail_unless(hc->codeInt() == HistCode::SystemCancel);
    fail_unless(hc->description() == std::string("System cancel"));
    fail_unless(hc->ediCode() == "79");
}END_TEST

START_TEST(check_hist_code_7)
{
    HistSubCode hc(HistCode::SetSac);

    fail_if(!hc, "HistSubCode not inited");

    fail_unless(hc->code() == std::string("SET_SAC"));
    fail_unless(hc->codeInt() == HistCode::SetSac);
    fail_unless(hc->description() == std::string("Set SAC"));
    fail_unless(hc->ediCode() == "142");
}END_TEST

START_TEST(check_monetary_type_1)
{
    MonetaryType it("IT");

    fail_unless(it->codeInt() == MonetaryType::It);
    fail_unless(it->code() == std::string("IT"));

    MonetaryType it2("NO ADC");
    it = it2;

    fail_unless(it->codeInt() == MonetaryType::NoADC);
    fail_unless(it->code() == std::string("NO ADC"));

    MonetaryType it3("†Ž€!");
    it = it3;

    fail_unless(it->codeInt() == MonetaryType::FreeText);
    fail_unless(it->code() == std::string("†Ž€!"));

    try {
        MonetaryType it_must_fail("‡„€‚‘’‚“‰ †Ž€ Ž‚›‰ ƒŽ„");
        fail_if(true, "MonetaryType('‡„€‚‘’‚“‰ †Ž€ Ž‚›‰ ƒŽ„')");
    }
    catch(TickExceptions::tick_soft_except &e) {
        // thats OK
        fail_unless(e.errCode() == EtErr::MISS_MONETARY_INF);
    }
    MonetaryType it4("†Ž€ 1234567890123");
    it = it4;

    fail_unless(it->codeInt() == MonetaryType::FreeText);
    fail_unless(it->code() == std::string("†Ž€ 1234567890123"));

    try {
        MonetaryType it_must_fail("†Ž€ 12345678901234");
        fail_if(true, "MonetaryType('†Ž€ 12345678901234')");
    }
    catch(TickExceptions::tick_soft_except &e) {
        // thats OK
        fail_unless(e.errCode() == EtErr::MISS_MONETARY_INF);
    }

}END_TEST

START_TEST(check_monetary_type_2)
{
    MonetaryType it(MonetaryType::It);

    fail_unless(it->codeInt() == MonetaryType::It);
    fail_unless(it->code() == std::string("IT"));

    MonetaryType it2(MonetaryType::NoADC);
    it = it2;

    fail_unless(it->codeInt() == MonetaryType::NoADC);
    fail_unless(it->code() == std::string("NO ADC"));

    MonetaryType it3(MonetaryType::FreeText);
    it = it3;

    fail_unless(it->codeInt() == MonetaryType::FreeText);
    fail_unless(it->code() == std::string(""));
//    fail_unless(it->code() == std::string(""));
}END_TEST

START_TEST(check_tax_amount_to_str)
{
    TaxAmount::Amount amount0d00("0.00");
    fail_unless(amount0d00.amStr() == std::string("0.00"));

    TaxAmount::Amount amount1d00("1.00");
    fail_unless(amount1d00.amStr() == std::string("1.00"));
    fail_unless(amount1d00.amStr(CutZeroFraction(true)) == std::string("1"));
    fail_unless(amount1d00.amStr(CutZeroFraction(true), MinFractionLength(2)) == std::string("1.00"));

    TaxAmount::Amount amount1d01("1.01");
    fail_unless(amount1d01.amStr() == std::string("1.01"));
    fail_unless(amount1d01.amStr(CutZeroFraction(true)) == std::string("1.01"));
    fail_unless(amount1d01.amStr(CutZeroFraction(true), MinFractionLength(2)) == std::string("1.01"));

}END_TEST

#define SUITENAME "eticklib"
TCASEREGISTER(0, 0)
{
    ADD_TEST(check_hist_code_1);
    ADD_TEST(check_hist_code_2);
    ADD_TEST(check_hist_code_3);
    ADD_TEST(check_hist_code_4);
    ADD_TEST(check_hist_code_5);
    ADD_TEST(check_hist_code_6);
    ADD_TEST(check_hist_code_7);
    ADD_TEST(check_monetary_type_1);
    ADD_TEST(check_monetary_type_2);
    ADD_TEST(check_tax_amount_to_str);
}
TCASEFINISH

}

#endif // XP_TESTING
