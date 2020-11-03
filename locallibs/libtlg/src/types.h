#ifndef TLGTYPES_H
#define TLGTYPES_H

/**
 * @file
 * @brief ���� ��� ࠡ��� � ��।�� ⥫��ࠬ
 * */

#ifdef __cplusplus
#include <string>
#include <boost/optional.hpp>

#include "hth.h"
#include "filter.h"
#include "consts.h"

/* 
 * @class TlgInfo
 * ᢮��⢠ ⥫��ࠬ�� �� ��।� queueType
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
 * @�lass INCOMING_INFO
 * @brief ������� ��� ��࠭���� ��襤�� ⥫��ࠬ�
 * */
struct INCOMING_INFO
{
    char pult[6+1];
    int q_num;              ///< ��।�, � ������ �⠢����
    int router;             ///< ����, � ���ண� ��諠 ⥫��ࠬ��
    int ttl;                ///< Time To Leave
    bool isEdifact;
    boost::optional<hth::HthInfo> hthInfo;
};

/**
 * @class OUT_INFO
 * @brief ������� ��� ��ࠢ�� ⥫��ࠬ��
 * */
struct OUT_INFO
{
    int q_num;          ///< ����� ����
    int ttl;            ///< TTL
    int isTpb;          ///< �ਧ��� TypeB
    int isExpress;      ///< �ਧ��� �����-⥫��ࠬ��
    int isHth;          ///< �ਧ��� H2H
    hth::HthInfo hthInfo;
    telegrams::tlg_text_filter filter;
};

#endif // __cplusplus
#endif /* TLGTYPES_H */

