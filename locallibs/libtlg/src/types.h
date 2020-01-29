#ifndef TLGTYPES_H
#define TLGTYPES_H

/**
 * @file
 * @brief Типы для работы с очередью телеграм
 * */

#ifdef __cplusplus
#include <string>
#include <boost/optional.hpp>

#include "hth.h"
#include "filter.h"
#include "consts.h"

/* 
 * @class TlgInfo
 * свойства телеграммы из очереди queueType
 * */
struct TlgInfo
{
    Direction queueType;
    union {
        int queueNum;
        int routerNum;
    };
    size_t ttl;
};

/**
 * @сlass INCOMING_INFO
 * @brief структура для сохранения пришедших телеграмм
 * */
struct INCOMING_INFO
{
    char pult[6+1];
    int q_num;              ///< очередь, в которую ставится
    int router;             ///< роутер, с которого пришла телеграмма
    int ttl;                ///< Time To Leave
    bool isEdifact;
    boost::optional<hth::HthInfo> hthInfo;
};

/**
 * @class OUT_INFO
 * @brief структура для отправки телеграммы
 * */
struct OUT_INFO
{
    int q_num;          ///< номер роутера
    int ttl;            ///< TTL
    int isTpb;          ///< признак TypeB
    int isExpress;      ///< признак экспресс-телеграммы
    int isHth;          ///< признак H2H
    hth::HthInfo hthInfo;
    telegrams::tlg_text_filter filter;
};

#endif // __cplusplus
#endif /* TLGTYPES_H */

