#ifndef _GET_CLS_UPGRADE_H_
#define _GET_CLS_UPGRADE_H_

#include <string>

enum TClsUpgradeType {cutUpgrade, cutDowngrade, cutNone};

TClsUpgradeType get_cls_upgrade(const std::string &cls, const std::string &compartment);

#endif
