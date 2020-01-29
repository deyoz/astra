#ifndef TLG_FUNC_H
#define TLG_FUNC_H

#ifdef __cplusplus
/**
 * @file
 * @brief Functions for working with telegram queues
 * */
struct tlgnum_t;
struct INCOMING_INFO;
extern "C" {
/**
 * �������� ⥫��ࠬ� � ��।� �� ��ࠡ��� 
 * @param ii - 㪠��⥫쭠 �������� INCOMING_INFO
 * @return ����� 㫮������ ⥫��ࠬ�, <0 �� �訡�� */
int write_tlg(tlgnum_t& num, INCOMING_INFO *ii, const char *body);

}

#endif

#endif /* TLG_FUNC_H */

