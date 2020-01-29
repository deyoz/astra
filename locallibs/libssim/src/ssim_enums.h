#pragma once

#include <serverlib/enum.h>

namespace ssim {

enum SsmActionType {
    SSM_INV, SSM_NEW, SSM_CNL, SSM_RPL, SSM_SKD,
    SSM_ACK, SSM_ADM, SSM_CON, SSM_EQT,
    SSM_FLT, SSM_NAC, SSM_REV, SSM_RSD, SSM_TIM
};

ENUM_NAMES_DECL(SsmActionType)

enum AsmActionType {
    ASM_INV, ASM_NEW, ASM_CNL, ASM_RIN, ASM_RPL,
    ASM_ACK, ASM_ADM, ASM_CON, ASM_EQT,
    ASM_FLT, ASM_NAC, ASM_RRT, ASM_TIM
};

ENUM_NAMES_DECL(AsmActionType)

} //ssim
