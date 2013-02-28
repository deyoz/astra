#ifndef _DEV_UTILS_H_
#define _DEV_UTILS_H_

#include "dev_consts.h"
#include <string>

ASTRA::TDevOperType DecodeDevOperType(std::string s);
ASTRA::TDevFmtType DecodeDevFmtType(std::string s);
std::string EncodeDevOperType(ASTRA::TDevOperType s);
std::string EncodeDevFmtType(ASTRA::TDevFmtType s);

//bcbp_begin_idx - ������ ��ࢮ�� ���� ����-����
//airline_use_begin_idx - ������ ��ࢮ�� ���� <For individual airline use> ��ࢮ�� ᥣ����
//airline_use_end_idx - ������, ᫥����� �� ��᫥���� ���⮬ <For individual airline use> ��ࢮ�� ᥣ����
void checkBCBP_M(const std::string bcbp,
                 const std::string::size_type bcbp_begin_idx,
                 std::string::size_type &airline_use_begin_idx,
                 std::string::size_type &airline_use_end_idx);

#endif
