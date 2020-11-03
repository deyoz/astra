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
 * Положить телеграму в очередь на обработку 
 * @param ii - указательна структуру INCOMING_INFO
 * @return номер уложенной телеграмы, <0 при ошибке */
int write_tlg(tlgnum_t& num, INCOMING_INFO *ii, const char *body);

}

#endif

#endif /* TLG_FUNC_H */

